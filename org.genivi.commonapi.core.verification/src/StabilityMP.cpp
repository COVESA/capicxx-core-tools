/* stability tests, control program. */
/* this program takes care of setting up a test and then making sure things happen in proper sequence. */

#include <functional>
#include <fstream>
#include <algorithm>
#include <gtest/gtest.h>
#include <CommonAPI/CommonAPI.hpp>
#include "v1_0/commonapi/stability/mp/TestInterfaceProxy.hpp"
#include "v1_0/commonapi/stability/mp/ControlInterfaceProxy.hpp"
#include "stub/StabControlStub.h"
#include "stub/StabilityMPStub.h"

#ifdef WIN32
#include <Windows.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#endif

using namespace v1_0::commonapi::stability::mp;

const std::string serviceId = "service-sample";
const std::string clientId = "client-sample";

const std::string domain = "local";
const std::string testAddress = "commonapi.stability.mp.TestInterface";
const std::string controlAddress = "commonapi.stability.mp.ControlInterface";
const std::string COMMONAPI_CONFIG_SUFFIX = ".conf";

// test magnitude constants - the bigger the numbers, the more stressful the test
const int N_SERVERS = 100;
const int N_THREADS = 100;

const int MAX_WAIT = 10000;

const int N_TEST_PROXY_PROCESSES = 3;
const int N_CHILDREN = 2 + 2 * N_TEST_PROXY_PROCESSES; // needs to be large enough to cover for all created processes

#ifdef WIN32
HANDLE childpids[N_CHILDREN];
#else
pid_t childpids[N_CHILDREN];
#endif

bool idChild;
bool controlServiceRegistered = false;
std::shared_ptr<CommonAPI::Runtime> runtime_;
std::shared_ptr<CommonAPI::Factory> stubFactory_;
std::shared_ptr<StabControlStub> controlServiceStub_;
class Environment: public :: testing::Environment {
public:
	virtual ~Environment() {
	}

	virtual void SetUp() {
	}

	virtual void TearDown() {
	}

};

// server states:
const int WAITING_FOR_CHILDREN = 0;
// commands from child to parent:
const uint32_t QUERY_NEW_ID = 0;
const uint32_t GET_COMMAND = 1;
const uint32_t CMD_DONE = 2;
const uint32_t PROXIES_CONNECTED_OK = 3;
const uint32_t PROXIES_CONNECTED_FAIL = 4;

// child states:
const int CHILD_SETUP = 1;
const int CHILD_IDLE = 2;
const int CHILD_RESERVED_IDLE = 3;
const int CHILD_PROCESSING_CMD = 4;
const int CONNECTION_STATUS_OK = 5;
const int CONNECTION_STATUS_FAIL = 6;
const int DEAD = 7;

// commands from parent to child
const uint8_t KEEP_IDLE = 1;
const uint8_t PROXY_CREATE = 2;
const uint8_t WAIT_UNTIL_CONNECTED = 3;
const uint8_t WAIT_UNTIL_DISCONNECTED = 4;
const uint8_t KILL_YOURSELF = 5;
const uint8_t SERVER_CREATE = 6;

class ControledChild {
public:
	uint8_t id;
	int state = CHILD_SETUP;
	uint8_t next_cmd;
};

std::vector<ControledChild *> children;
bool allChildrenRegistered = false;

bool isFree(ControledChild * pChild)
{
	return (pChild->state == CHILD_IDLE);
}
ControledChild * findFreeChild(void)
{
	std::vector<ControledChild *>::iterator i = std::find_if(children.begin(), children.end(), isFree);
	if (i == children.end())
		return 0;
	(*i)->state = CHILD_RESERVED_IDLE;
	return *i;
}
ControledChild * findChild(uint8_t id)
{

	std::vector<ControledChild *>::iterator i = children.begin();
	while (i != children.end()) {
		if ((*i)->id == id)
			return *i;
		i++;
	}
	return 0;
}

class ProxyThread {
public:
	std::shared_ptr<TestInterfaceProxy<>> proxy_[N_SERVERS];
	void createProxies(void) {
		// create a proxy for each of the servers
		for (unsigned int proxycount = 0; proxycount < N_SERVERS; proxycount++) {
			proxy_[proxycount] = runtime_->buildProxy<TestInterfaceProxy>(domain, testAddress + std::to_string(proxycount), clientId);
			success_ = success_ && (bool)proxy_[proxycount];
		}
	}
	void pollForAvailability(void) {
		success_ = false;
		for (int wait = 0; wait < 100; wait++) {
			bool allAvailable = true;
			for (unsigned int proxycount = 0; proxycount < N_SERVERS; proxycount++) {
				if (!proxy_[proxycount]->isAvailable()) {
					allAvailable = false;
					break;
				}
			}
			if (allAvailable) {
				success_ = true;
				break;
			}
			usleep(20000);
		}
	}
	void pollForUnavailability(void) {
		success_ = false;
		for (int wait = 0; wait < 100; wait++) {
			bool allUnAvailable = true;
			for (unsigned int proxycount = 0; proxycount < N_SERVERS; proxycount++) {
				if (proxy_[proxycount]->isAvailable()) {
					allUnAvailable = false;
					break;
				}
			}
			if (allUnAvailable) {
				success_ = true;
				break;
			}
			usleep(10000);
		}
	}
	void setThread(std::thread *thread) {
		thread_ = thread;
	}
	std::thread * getThread(void) {
		return thread_;
	}
	bool getSuccess(void) {
		return success_;
	}
	std::thread *thread_ = 0;
	bool success_ = true;
};

class ChildProcess {
public:
	bool setup()
	{
		runtime_ = CommonAPI::Runtime::get();
		testProxy_ = runtime_->buildProxy<ControlInterfaceProxy>(domain, controlAddress, clientId);
		while(!testProxy_->isAvailable())
			usleep(10000);
		// send register message through command interface
		uint8_t cmd;
		uint32_t data1, data2;
		bool status = sendMessage(0, QUERY_NEW_ID, cmd, data1, data2);
		if (!status)
			// problems with communication.
			return false;
		id = cmd;
		state = CHILD_IDLE;
		return true;
	}

	bool sendMessage(const uint8_t &id,
			const uint32_t &data,
			uint8_t &command,
			uint32_t &data1,
			uint32_t &data2
	)
	{
		CommonAPI::CallStatus callStatus;
		testProxy_->controlMethod(id, data, callStatus, command, data1, data2);
		if (callStatus == CommonAPI::CallStatus::SUCCESS)
			return true;
		// on error, exit - the control service has most likely just died.
		exit(1);
		return false;
	}

	void processCommands()
	{
		ProxyThread * proxyrunners[N_THREADS];
		while (true) {
			switch(state)
			{
			case CHILD_IDLE:
				// send polling message
				uint8_t cmd;
				uint32_t data1, data2;
				bool status = sendMessage(id, GET_COMMAND, cmd, data1, data2);
				if (cmd == KEEP_IDLE) {
					usleep(10000);
					break;
				}
				if (cmd == PROXY_CREATE) {
					// create threads with proxies to test services.
					for (unsigned int threadcount = 0; threadcount < N_THREADS; threadcount++) {
						proxyrunners[threadcount] = new ProxyThread();
						std::thread * thread = new std::thread(std::bind(&ProxyThread::createProxies, proxyrunners[threadcount]));
						proxyrunners[threadcount]->setThread(thread);
					}

					// wait until all threads have completed creating the proxies.
					for (unsigned int threadcount = 0; threadcount < N_THREADS; threadcount++) {
						proxyrunners[threadcount]->getThread()->join();
						delete proxyrunners[threadcount]->getThread();
						proxyrunners[threadcount]->setThread(0);
					}

					// once done with all, send CMD_DONE message.
					sendMessage(id, CMD_DONE, cmd, data1, data2);
					// then start polling again.
					state = CHILD_IDLE;
				}
				else if (cmd == SERVER_CREATE) {
					// create services for a number of addresses
					std::shared_ptr<StabilityMPStub> testMultiRegisterStub_;
					testMultiRegisterStub_ = std::make_shared<StabilityMPStub>();
					for (unsigned int regcount = 0; regcount < N_SERVERS; regcount++) {
						bool serviceRegistered_ = runtime_->registerService(domain, testAddress + std::to_string( regcount ), testMultiRegisterStub_, serviceId);
					}

					sendMessage(id, CMD_DONE, cmd, data1, data2);
					state = CHILD_IDLE;
				}
				else if (cmd == WAIT_UNTIL_CONNECTED) {
					// create threads to test the proxy availability
					for (unsigned int threadcount = 0; threadcount < N_THREADS; threadcount++) {
						std::thread * thread = new std::thread(std::bind(&ProxyThread::pollForAvailability, proxyrunners[threadcount]));
						proxyrunners[threadcount]->setThread(thread);
					}
					// wait until all threads have completed
					for (unsigned int threadcount = 0; threadcount < N_THREADS; threadcount++) {
						proxyrunners[threadcount]->getThread()->join();
						delete proxyrunners[threadcount]->getThread();
						proxyrunners[threadcount]->setThread(0);
					}
					// check for succession status
					bool success = true;
					for (unsigned int threadcount = 0; threadcount < N_THREADS; threadcount++) {
						success = success & proxyrunners[threadcount]->getSuccess();
					}

					// send message with the succession status
					if (success)
						sendMessage(id, PROXIES_CONNECTED_OK, cmd, data1, data2);
					else
						sendMessage(id, PROXIES_CONNECTED_FAIL, cmd, data1, data2);

					// then start polling again.
					state = CHILD_IDLE;

				}
				else if (cmd == WAIT_UNTIL_DISCONNECTED) {
					// create threads to test the proxy unavailability
					for (unsigned int threadcount = 0; threadcount < N_THREADS; threadcount++) {
						std::thread * thread = new std::thread(std::bind(&ProxyThread::pollForUnavailability, proxyrunners[threadcount]));
						proxyrunners[threadcount]->setThread(thread);
					}
					// wait until all threads have completed
					for (unsigned int threadcount = 0; threadcount < N_THREADS; threadcount++) {
						proxyrunners[threadcount]->getThread()->join();
						delete proxyrunners[threadcount]->getThread();
						proxyrunners[threadcount]->setThread(0);
					}
					// check for succession status
					bool success = true;
					for (unsigned int threadcount = 0; threadcount < N_THREADS; threadcount++) {
						success = success & proxyrunners[threadcount]->getSuccess();
					}

					// send message with the succession status
					if (success)
						sendMessage(id, PROXIES_CONNECTED_OK, cmd, data1, data2);
					else
						sendMessage(id, PROXIES_CONNECTED_FAIL, cmd, data1, data2);

					// then start polling again.
					state = CHILD_IDLE;

				}
				else if (cmd == KILL_YOURSELF) {
					exit(0);
				}

			}
		}
		return;
	}


	std::shared_ptr<ControlInterfaceProxy<>> testProxy_;

	uint8_t id = 0;
	int state = CHILD_SETUP;
};

int state = WAITING_FOR_CHILDREN;
static int8_t next_id = 1;

void listener(uint8_t id,uint32_t data, uint8_t& command, uint32_t& min, uint32_t &max) {
	command = 0;
	min = 0;
	max = 0;
	switch(data) {
	case QUERY_NEW_ID:
	{
		// create a child data structure
		ControledChild * child = new ControledChild();

		// add this to the structure
		child->id = next_id;
		// increment id
		next_id++;
		child->state = CHILD_IDLE;
		// prepare to return id to child
		command = child->id;
		children.push_back(child);
		child->next_cmd = KEEP_IDLE;
	}
	if (children.size() == N_CHILDREN)
		allChildrenRegistered = true;
	break;

	case GET_COMMAND:
	{
		// find the child with this id
		ControledChild * child = findChild(id);
		ASSERT_TRUE((bool)child);
		uint8_t cmd = child->next_cmd;
		if (cmd != KEEP_IDLE) {
			child->next_cmd = KEEP_IDLE;
			child->state = CHILD_PROCESSING_CMD;
		}
		command = cmd;
	}
	break;

	case CMD_DONE:
	{
		// find the child with this id
		ControledChild * child = findChild(id);
		ASSERT_TRUE((bool)child);
		child->state = CHILD_RESERVED_IDLE;
	}
	break;

	case PROXIES_CONNECTED_OK:
	{
		// find the child with this id
		ControledChild * child = findChild(id);
		ASSERT_TRUE((bool)child);
		child->state = CONNECTION_STATUS_OK;
	}
	break;

	case PROXIES_CONNECTED_FAIL:
	{
		// find the child with this id
		ControledChild * child = findChild(id);
		ASSERT_TRUE((bool)child);
		child->state = CONNECTION_STATUS_FAIL;
	}
	break;
	}
}


/* this class is instantiated only for the control process. */
class Stability: public ::testing::Test {
protected:
	void SetUp() {

	}

	void TearDown() {

	}
};

TEST_F(Stability, BasicFunctionality) {
	ASSERT_TRUE(allChildrenRegistered);
}


TEST_F(Stability, ProxyCreation_ProxyFirst) {
	/*
	DO for N child processes
		get free child entry (proxy process)
		setup 'proxy c' command
		wait until child proc is in 'CMD DONE'
	END
	get free child entry (server)
	setup 'server c' command
	wait until server is in 'CMD DONE'
	DO for N proxy processes
		setup 'query status' c
		wait until child proc is in 'CMD DONE'
		assert child proc status is okay
	END
	setup 'kill' command to server
	wait until server is in 'CMD SENT'
	DO for N proxy processes
		setup 'query status' cmd
		wait until child proc is in 'CMD DONE'
		assert child proc status is okay
		setup 'kill' to child
		wait until child proc is in CMD SENT
	END
	 */
	ControledChild * proxyprocs[N_TEST_PROXY_PROCESSES];
	for (int i = 0; i < N_TEST_PROXY_PROCESSES; i++) {
		ControledChild * child = findFreeChild();
		ASSERT_TRUE((bool)child);
		proxyprocs[i] = child;
		child->state = CHILD_PROCESSING_CMD;
		child->next_cmd = PROXY_CREATE;
	}
	// wait until children are ready for more commands
	for (int i = 0; i < N_TEST_PROXY_PROCESSES; i++) {
		ControledChild * child = proxyprocs[i];
		for (int wait = 0; wait < MAX_WAIT; wait++) {
			if (child->state == CHILD_RESERVED_IDLE) {
				break;
			}
			usleep(100000);
		}
		ASSERT_TRUE(child->state == CHILD_RESERVED_IDLE);
	}
	// setup server test proc
	ControledChild * srvChild = findFreeChild();
	ASSERT_TRUE((bool)srvChild);
	srvChild->state = CHILD_PROCESSING_CMD;
	srvChild->next_cmd = SERVER_CREATE;
	for (int wait = 0; wait < MAX_WAIT; wait++) {
		if (srvChild->state == CHILD_RESERVED_IDLE) {
			break;
		}
		usleep(100000);
	}
	ASSERT_TRUE(srvChild->state == CHILD_RESERVED_IDLE);
	// send proxy status query messages
	for (int i = 0; i < N_TEST_PROXY_PROCESSES; i++) {
		ControledChild * child = proxyprocs[i];
		child->state = CHILD_PROCESSING_CMD;
		child->next_cmd = WAIT_UNTIL_CONNECTED;
	}
	// wait until children return connection status
	for (int i = 0; i < N_TEST_PROXY_PROCESSES; i++) {
		ControledChild * child = proxyprocs[i];
		for (int wait = 0; wait < MAX_WAIT; wait++) {
			if (child->state == CONNECTION_STATUS_OK) {
				break;
			}
			if (child->state == CONNECTION_STATUS_FAIL) {
				break;
			}
			usleep(100000);
		}
		ASSERT_TRUE(child->state == CONNECTION_STATUS_OK);
	}
	// kill server
	srvChild->next_cmd = KILL_YOURSELF;
	srvChild->state = DEAD;
	// send proxy status query messages
	for (int i = 0; i < N_TEST_PROXY_PROCESSES; i++) {
		ControledChild * child = proxyprocs[i];
		child->next_cmd = WAIT_UNTIL_DISCONNECTED;
	}
	// wait until children return connection status
	for (int i = 0; i < N_TEST_PROXY_PROCESSES; i++) {
		ControledChild * child = proxyprocs[i];
		for (int wait = 0; wait < MAX_WAIT; wait++) {
			if (child->state == CONNECTION_STATUS_OK) {
				break;
			}
			if (child->state == CONNECTION_STATUS_FAIL) {
				break;
			}
			usleep(100000);
		}
		ASSERT_TRUE(child->state == CONNECTION_STATUS_OK);
	}
	// kill children
	for (int i = 0; i < N_TEST_PROXY_PROCESSES; i++) {
		ControledChild * child = proxyprocs[i];
		child->next_cmd = KILL_YOURSELF;
		child->state = DEAD;
	}

	usleep(1 * 1000 * 1000);
}

TEST_F(Stability, ProxyCreation_ServerFirst) {

	ControledChild * proxyprocs[N_TEST_PROXY_PROCESSES];

	// setup server test proc
	ControledChild * srvChild = findFreeChild();
	ASSERT_TRUE((bool)srvChild);
	srvChild->state = CHILD_PROCESSING_CMD;
	srvChild->next_cmd = SERVER_CREATE;
	for (int wait = 0; wait < MAX_WAIT; wait++) {
		if (srvChild->state == CHILD_RESERVED_IDLE) {
			break;
		}
		usleep(100000);
	}
	ASSERT_TRUE(srvChild->state == CHILD_RESERVED_IDLE);

	for (int i = 0; i < N_TEST_PROXY_PROCESSES; i++) {
		ControledChild * child = findFreeChild();
		ASSERT_TRUE((bool)child);
		proxyprocs[i] = child;
		child->state = CHILD_PROCESSING_CMD;
		child->next_cmd = PROXY_CREATE;
	}
	// wait until children are ready for more commands
	for (int i = 0; i < N_TEST_PROXY_PROCESSES; i++) {
		ControledChild * child = proxyprocs[i];
		for (int wait = 0; wait < MAX_WAIT; wait++) {
			if (child->state == CHILD_RESERVED_IDLE) {
				break;
			}
			usleep(100000);
		}
		ASSERT_TRUE(child->state == CHILD_RESERVED_IDLE);
	}

	// send proxy status query messages
	for (int i = 0; i < N_TEST_PROXY_PROCESSES; i++) {
		ControledChild * child = proxyprocs[i];
		child->state = CHILD_PROCESSING_CMD;
		child->next_cmd = WAIT_UNTIL_CONNECTED;
	}
	// wait until children return connection status
	for (int i = 0; i < N_TEST_PROXY_PROCESSES; i++) {
		ControledChild * child = proxyprocs[i];
		for (int wait = 0; wait < MAX_WAIT; wait++) {
			if (child->state == CONNECTION_STATUS_OK) {
				break;
			}
			if (child->state == CONNECTION_STATUS_FAIL) {
				break;
			}
			usleep(100000);
		}
		ASSERT_TRUE(child->state == CONNECTION_STATUS_OK);
	}
	// kill server
	srvChild->next_cmd = KILL_YOURSELF;
	srvChild->state = DEAD;
	// send proxy status query messages
	for (int i = 0; i < N_TEST_PROXY_PROCESSES; i++) {
		ControledChild * child = proxyprocs[i];
		child->next_cmd = WAIT_UNTIL_DISCONNECTED;
	}
	// wait until children return connection status
	for (int i = 0; i < N_TEST_PROXY_PROCESSES; i++) {
		ControledChild * child = proxyprocs[i];
		for (int wait = 0; wait < MAX_WAIT; wait++) {
			if (child->state == CONNECTION_STATUS_OK) {
				break;
			}
			if (child->state == CONNECTION_STATUS_FAIL) {
				break;
			}
			usleep(100000);
		}
		ASSERT_TRUE(child->state == CONNECTION_STATUS_OK);
	}
	// kill children
	for (int i = 0; i < N_TEST_PROXY_PROCESSES; i++) {
		ControledChild * child = proxyprocs[i];
		child->next_cmd = KILL_YOURSELF;
		child->state = DEAD;
	}

	usleep(1 * 1000 * 1000);
}

int main(int argc, char ** argv)
{
	/* create the necessary child processes */
	/* forking is best done before the google test environment is set up */
	bool isChild = false;
	for (int i = 0; i < N_CHILDREN; i++) {
#ifdef WIN32
		// TODO!!!
		HANDLE child = 0;
#else
		pid_t child = fork();
#endif
		if (child == 0) {
			isChild = true;
			break;
		}
		childpids[i] = child;
	}

	/* if we are not a child, set up test environment */
	if (!isChild) {
		// do this just once
		runtime_ = CommonAPI::Runtime::get();
		// register control service
		StabControlStub::registerListener(&listener);
		controlServiceStub_ = std::make_shared<StabControlStub>();
		controlServiceRegistered = runtime_->registerService(domain, controlAddress, controlServiceStub_, serviceId);
		std::cout << "Control service registered at " << controlAddress << std::endl;

		// wait until children have been registered
		while (!allChildrenRegistered) {
			usleep(10000);
		}
		::testing::InitGoogleTest(&argc, argv);
		::testing::AddGlobalTestEnvironment(new Environment());
		return RUN_ALL_TESTS();
	}
	/* otherwise, start acting as a child process */
	ChildProcess child;
	child.setup();
	child.processCommands();

	/* TBD */

	return 0;
}



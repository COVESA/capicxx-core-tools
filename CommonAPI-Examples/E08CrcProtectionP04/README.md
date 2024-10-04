# Example 8: CRC Protection Using Profile P04

This example was created to demonstrate how the e2e implementation can be used on the CommonAPI layer with profile P04. Please note that this example can only be correctly used if the Service and the Client are running on distinct machines/ECUs.
First consider the Franca IDL specification of example 8 **E08CrcProtectionP04.fidl**:

```java
package commonapi.examples

interface E08CrcProtectionP04 {
    version { major 1 minor 0 }

    attribute CommonTypes.aStruct aOK
    attribute CommonTypes.aStruct aERROR
}

typeCollection CommonTypes {
    version { major 1 minor 0 }
    
    struct aStruct {
          UInt16 CommonCRCLength 
          UInt16 CommonCRCCounter
          UInt32 CommonCRCID
          UInt32 CommonCRC 
          UInt16 value1
          UInt16 value2
    }
}
```

The following CRC related fields in the *aStruct* (*CommonCRCLength*, *CommonCRCCounter*, *CommonCRCID*, *CommonCRC*) are automatically computed and setted using the required e2e definitions on the .json file.
In this example we will demonstrate how distinct e2e definitions on the configuration file for the service and the client for the *aERROR* attribute lead to a *CallStatus::INVALID_VALUE* on a changed event notification, after a subscription.

Let's start by the configuration file used for the service **vsomeip-service.json**:

```json
{
    "unicast" : "192.168.0.2",
    "logging" :
    { 
        "level" : "debug",
        "console" : "true",
        "file" : { "enable" : "false", "path" : "/tmp/vsomeip.log" },
        "dlt" : "false"
    },
    "applications" :
    [
        {
            "name" : "service-sample",
            "id" : "0x1277"
        }
    ],
    "services" :
    [
        {
            "service" : "0x1234",
            "instance" : "0x0001",
            "reliable" : { "port" : "30499", "enable-magic-cookies" : "false" }
        }
    ],
    "e2e" :
    {
        "e2e_enabled" : "true",
        "protected" :
        [
            {
                "service_id" : "0x1234",
                "event_id" : "0x80e9",
                "profile" : "P04",
                "variant" : "protector",
                "crc_offset" : "64",
                "data_id" : "0xFF"
            },
            {
                "service_id" : "0x1234",
                "event_id" : "0x80ea",
                "profile" : "P04",
                "variant" : "protector",
                "crc_offset" : "64",
                "data_id" : "0xFF"
            }
        ]
    },
    "routing" : "service-sample",
    "service-discovery" :
    {
        "enable" : "true",
        "multicast" : "224.244.224.245",
        "port" : "30490",
        "protocol" : "udp"
    }
}
```

And bellow the configuration file used for the client **vsomeip-client.json**:

```json
{
    "unicast" : "10.0.3.1",
    "netmask" : "255.255.255.0",
    "logging" : 
    {
        "level" : "debug",
        "console" : "true",
        "file" : { "enable" : "true", "path" : "/var/log/vsomeip.log" },
        "dlt" : "true"
    },
    "applications" :
    [
        {
            "name" : "client-sample",
            "id" : "0x1343"
        }
    ],
    "e2e" :
    {
        "e2e_enabled" : "true",
        "protected" :
        [
            {
                "service_id" : "0x1234",
                "event_id" : "0x80e9",
                "profile" : "P04",
                "variant" : "checker",
                "crc_offset" : "64",
                "data_id" : "0xFF"
            },
            {
                "service_id" : "0x1234",
                "event_id" : "0x80ea",
                "profile" : "P04",
                "variant" : "checker",
                "crc_offset" : "64",
                "data_id" : "0x00"
            }
        ]
    },
    "routing" : "client-sample",
    "service-discovery" :
    {
        "enable" : "true",
        "multicast" : "224.244.224.245",
        "port" : "30490",
        "protocol" : "udp"
    }
}
```

Between both it is possible to see, in the e2e definitions, that the *data_id* for the *aERROR* attribute (*event_id 0x80ea*) has distinct values between the service and the client, *0xFF* vs *0x00*, while the definitions for the *aOK* attribute (*event_id 0x80e9*) match.
Please note that the first definition, *unicast* will need to be adapted to your specific setup.

Now let's have a look to the CommonAPI code on the service side, that is splitted between the following 3 files:

* **E08CrcProtectionP04StubImpl.hpp**:
```cpp
#ifndef E08CRCPROTECTIONP04STUBIMPL_HPP_
#define E08CRCPROTECTIONP04STUBIMPL_HPP_

#include <CommonAPI/CommonAPI.hpp>
#include <v1/commonapi/examples/E08CrcProtectionP04StubDefault.hpp>

class E08CrcProtectionP04StubImpl: public v1_0::commonapi::examples::E08CrcProtectionP04StubDefault {

public:
    E08CrcProtectionP04StubImpl();
    virtual ~E08CrcProtectionP04StubImpl();
    virtual void incCounter();

private:
    int cnt;
};

#endif // E08CRCPROTECTIONP04STUBIMPL_HPP_
```

* **E08CrcProtectionP04StubImpl.cpp**:
```cpp
#include "E08CrcProtectionP04StubImpl.hpp"

using namespace v1::commonapi::examples;

E08CrcProtectionP04StubImpl::E08CrcProtectionP04StubImpl() {
    cnt = 0;
}

E08CrcProtectionP04StubImpl::~E08CrcProtectionP04StubImpl() {
}

void E08CrcProtectionP04StubImpl::incCounter() {
    cnt++;

    CommonTypes::aStruct aOK, aERROR;
    aOK.setValue1((uint16_t)cnt);
    aOK.setValue2((uint16_t)cnt+1);
    setAOKAttribute(aOK);

    aERROR.setValue1((uint16_t)cnt);
    aERROR.setValue2((uint16_t)cnt+2);
    setAERRORAttribute(aERROR);
    std::cout << "New counter value = " << cnt << "!" << std::endl;
}
```

* **E08CrcProtectionP04Service.cpp**:
```cpp
#include <iostream>
#include <thread>

#include <CommonAPI/CommonAPI.hpp>
#include "E08CrcProtectionP04StubImpl.hpp"

int main() {
    CommonAPI::Runtime::setProperty("LogContext", "E08S");
    CommonAPI::Runtime::setProperty("LogApplication", "E08S");
    CommonAPI::Runtime::setProperty("LibraryBase", "E08CrcProtectionP04");

    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();

    std::string domain = "local";
    std::string instance = "commonapi.examples.Attributes";
    std::string connection = "service-sample";

    std::shared_ptr<E08CrcProtectionP04StubImpl> myService = std::make_shared<E08CrcProtectionP04StubImpl>();
    while (!runtime->registerService(domain, instance, myService, connection)) {
        std::cout << "Register Service failed, trying again in 100 milliseconds..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "Successfully Registered Service!" << std::endl;

    while (true) {
        myService->incCounter(); // Change value of attributes, see stub implementation
        std::cout << "Waiting for calls... (Abort with CTRL+C)" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    return 0;
}
```

On these implementations we can see that the service side starts by registering a service and if successful, starts incrementing a counter every 5 seconds that is used to set new values into the attributes *aOK* and *aERROR*. Notice that none of the CRC fields are setted explicitly.

Now, we move into the implementation for the client side in **E08CrcProtectionP04Client.cpp**:
```cpp
#include <iostream>
#include <thread>

#ifndef _WIN32
#include <unistd.h>
#endif

#include <CommonAPI/CommonAPI.hpp>
#include <CommonAPI/AttributeCacheExtension.hpp>
#include <v1/commonapi/examples/E08CrcProtectionP04Proxy.hpp>

using namespace v1::commonapi::examples;

int main() {
    CommonAPI::Runtime::setProperty("LogContext", "E08C");
    CommonAPI::Runtime::setProperty("LogApplication", "E08C");
    CommonAPI::Runtime::setProperty("LibraryBase", "E08CrcProtectionP04");

    std::shared_ptr < CommonAPI::Runtime > runtime = CommonAPI::Runtime::get();

    std::string domain = "local";
    std::string instance = "commonapi.examples.Attributes"; 
    std::string connection = "client-sample";

    auto myProxy = runtime->buildProxyWithDefaultAttributeExtension<E08CrcProtectionP04Proxy, CommonAPI::Extensions::AttributeCacheExtension>(domain, instance, connection);

    std::cout << "Waiting for service to become available." << std::endl;
    while (!myProxy->isAvailable()) {
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }

    CommonTypes::aStruct aOK, aERROR;
    CommonAPI::CallInfo info(1000);
    info.sender_ = 1;

    // Subscribe aOK for receiving values
    myProxy->getAOKAttribute().getChangedEvent().subscribe(
        [&](const CommonTypes::aStruct& val) {
            std::cout << "Received change message on aOK:" << std::endl
                      << "\tCommonCRCLength: " << val.getCommonCRCLength() << std::endl
                      << "\tCommonCRCCounter: " << val.getCommonCRCCounter() << std::endl
                      << "\tCommonCRCID: " << val.getCommonCRCID() << std::endl
                      << "\tCommonCRC: " << val.getCommonCRC() << std::endl
                      << "\tvalue1: " << val.getValue1() << std::endl
                      << "\tvalue2: " << val.getValue2() << std::endl;
        },
        [&](const CommonAPI::CallStatus &status) {
             if (status == CommonAPI::CallStatus::INVALID_VALUE) {
                std::cout << "Subscription (Changed Event) of aOK attribute returned CallStatus==INVALID_VALUE" << std::endl;
                // do something... (no eventhandler gets called in case of INVALID_VALUE status) 
            } else  if (status == CommonAPI::CallStatus::SUCCESS) {
                std::cout << "Got valid response for aOK Async getter" << std::endl;
            }
        }
    );

    // Subscribe aERROR for receiving values
    myProxy->getAERRORAttribute().getChangedEvent().subscribe(
        [&](const CommonTypes::aStruct& val) {
            std::cout << "Received change message on aERROR:" << std::endl
                      << "\tCommonCRCLength: " << val.getCommonCRCLength() << std::endl
                      << "\tCommonCRCCounter: " << val.getCommonCRCCounter() << std::endl
                      << "\tCommonCRCID: " << val.getCommonCRCID() << std::endl
                      << "\tCommonCRC: " << val.getCommonCRC() << std::endl
                      << "\tvalue1: " << val.getValue1() << std::endl
                      << "\tvalue2: " << val.getValue2() << std::endl;
        },
        [&](const CommonAPI::CallStatus &status) {
             if (status == CommonAPI::CallStatus::INVALID_VALUE) {
                std::cout << "Subscription (Changed Event) of aERROR attribute returned CallStatus==INVALID_VALUE" << std::endl;
                // do something... (no eventhandler gets called in case of INVALID_VALUE status) 
            } else  if (status == CommonAPI::CallStatus::SUCCESS) {
                std::cout << "Got valid response for aERROR Async getter" << std::endl;
            }
        }
    );

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
```

The client, after a successful build of the proxy, uses the *getAOKAttribute* and *getAERRORAttribute* methods to subscribe to change events on the attributes *aOK* and *aERROR*, respectively. Every notification/access returns a flag named *callStatus* (please see the CommonAPI specification), that has the value *SUCCESS* if everything is working as supposed or a different value if not, in this case, it returns an *INVALID_VALUE* if the CRC values do not match between the computed values for the client and for the service.

Next we will present some logs of the execution of the service and the client on different machines, respectively:
```bash
bmw:/mnt/E08CrcProtectionP04$ VSOMEIP_CONFIGURATION=vsomeip-service.json COMMONAPI_CONFIG=$PWD/commonapi4someip.ini LD_LIBRARY_PATH=<path-to-install-folder>/lib:$PWD/build   build/E08CrcProtectionP04Service
[CAPI][INFO] Loading configuration file '/mnt/E08CrcProtectionP04/commonapi4someip.ini'
[CAPI][INFO] Using default binding 'someip'
[CAPI][INFO] Using default shared library folder '/usr/local/lib/commonapi'
[CAPI][DEBUG] Loading library for local:commonapi.examples.E08CrcProtectionP04:v1_0:commonapi.examples.Attributes stub.
[CAPI][INFO] Loading configuration file /etc//commonapi-someip.ini
[CAPI][DEBUG] Added address mapping: local:commonapi.examples.E08CrcProtectionP04:v1_0:commonapi.examples.Attributes <--> [1234.0001(1.0)]
[CAPI][VERBOSE] Registering function for creating "commonapi.examples.E08CrcProtectionP04:v1_0" proxy.
[CAPI][INFO] Registering function for creating "commonapi.examples.E08CrcProtectionP04:v1_0" stub adapter.
[CAPI][DEBUG] Loading interface library "libE08CrcProtectionP04-someip.so" succeeded.
[CAPI][INFO] Registering stub for "local:commonapi.examples.E08CrcProtectionP04:v1_0:commonapi.examples.Attributes"
[326189.926872]~DLT~  135~INFO     ~FIFO /tmp/dlt cannot be opened. Retrying later...
2022-09-12 15:10:32.092482 [info] Parsed vsomeip configuration in 2ms
2022-09-12 15:10:32.093414 [info] Using configuration file: "/mnt/E08CrcProtectionP04/vsomeip-service.json".
2022-09-12 15:10:32.093760 [info] Configuration module loaded.
2022-09-12 15:10:32.093986 [info] Initializing vsomeip application "service-sample".
2022-09-12 15:10:32.094746 [info] Instantiating routing manager [Host].
2022-09-12 15:10:32.096105 [info] create_routing_root: Local routing @ /tmp/vsomeip-0
2022-09-12 15:10:32.097361 [info] Service Discovery enabled. Trying to load module.
2022-09-12 15:10:32.104329 [info] Service Discovery module loaded.
2022-09-12 15:10:32.104857 [info] E2E protection enabled.
2022-09-12 15:10:32.110159 [info] E2E module loaded.
2022-09-12 15:10:32.110622 [info] vsomeip tracing not enabled. . vsomeip service discovery tracing not enabled.
2022-09-12 15:10:32.110774 [info] Application(service-sample, 1277) is initialized (11, 100).
2022-09-12 15:10:32.111913 [info] Starting vsomeip application "service-sample" (1277) using 2 threads I/O nice 255
2022-09-12 15:10:32.113024 [info] main dispatch thread id from application: 1277 (service-sample) is: 7fe1d249a700 TID: 141
2022-09-12 15:10:32.113515 [info] shutdown thread id from application: 1277 (service-sample) is: 7fe1d1c99700 TID: 142
2022-09-12 15:10:32.114162 [info] offer_eventEvent [1234.0001.80e9.] cycle time: 0
2022-09-12 15:10:32.114572 [info] REGISTER EVENT(1277): [1234.0001.80e9:is_provider=true]
2022-09-12 15:10:32.114988 [info] Watchdog is disabled!
2022-09-12 15:10:32.115964 [info] offer_eventEvent [1234.0001.80ea.] cycle time: 0
2022-09-12 15:10:32.116259 [info] io thread id from application: 1277 (service-sample) is: 7fe1d2c9b700 TID: 140
2022-09-12 15:10:32.116247 [info] io thread id from application: 1277 (service-sample) is: 7fe1d0c97700 TID: 144
2022-09-12 15:10:32.116362 [info] REGISTER EVENT(1277): [1234.0001.80ea:is_provider=true]
2022-09-12 15:10:32.117399 [info] vSomeIP 3.2.19 | (default)
2022-09-12 15:10:32.117708 [info] OFFER(1277): [1234.0001:1.0] (true)
2022-09-12 15:10:32.118129 [info] Network interface "eth0" state changed: up
2022-09-12 15:10:32.118839 [info] create_local_server: Listening @ /tmp/vsomeip-1277
2022-09-12Successfully Registered Service!
15:10:32.119157 [info] Route "default route (0.0.0.0/0) if: eth0 gw: 10.0.3.1" state changed: up
New counter value = 1!
Waiting for calls... (Abort with CTRL+C)
2022-09-12 15:10:32.124982 [debug] Joining to multicast group 224.244.224.245 from 10.0.3.5
2022-09-12 15:10:32.126384 [info] SOME/IP routing ready.
New counter value = 2!
Waiting for calls... (Abort with CTRL+C)
2022-09-12 15:10:42.122139 [info] vSomeIP 3.2.19 | (default)
New counter value = 3!
Waiting for calls... (Abort with CTRL+C)
2022-09-12 15:10:44.210599 [info] REMOTE SUBSCRIBE(0000): [1234.0001.0001] from 10.0.3.1:45025 reliable was accepted
2022-09-12 15:10:44.215350 [info] REMOTE SUBSCRIBE(0000): [1234.0001.0002] from 10.0.3.1:45025 reliable was accepted
New counter value = 4!
Waiting for calls... (Abort with CTRL+C)
```

```bash
bmw:~/Documents/commonapi-core-generator/CommonAPI-Examples/E08CrcProtectionP04$ VSOMEIP_CONFIGURATION=vsomeip-client.json COMMONAPI_CONFIG=$PWD/commonapi4someip.ini LD_LIBRARY_PATH=../../../install_folder/lib:$PWD/build   build/E08CrcProtectionP04Client
[326380.518559]~DLT~2809822~INFO     ~FIFO /tmp/dlt cannot be opened. Retrying later...
[CAPI][INFO] Loading configuration file '/home/bmw/Documents/commonapi-core-generator/CommonAPI-Examples/E08CrcProtectionP04/commonapi4someip.ini'
[CAPI][INFO] Using default binding 'someip'
[CAPI][INFO] Using default shared library folder '/usr/local/lib/commonapi'
[CAPI][DEBUG] Loading library for local:commonapi.examples.E08CrcProtectionP04:v1_0:commonapi.examples.Attributes proxy.
[CAPI][INFO] Loading configuration file /etc//commonapi-someip.ini
[CAPI][DEBUG] Added address mapping: local:commonapi.examples.E08CrcProtectionP04:v1_0:commonapi.examples.Attributes <--> [1234.0001(1.0)]
[CAPI][VERBOSE] Registering function for creating "commonapi.examples.E08CrcProtectionP04:v1_0" proxy.
[CAPI][INFO] Registering function for creating "commonapi.examples.E08CrcProtectionP04:v1_0" stub adapter.
[CAPI][DEBUG] Loading interface library "libE08CrcProtectionP04-someip.so" succeeded.
[CAPI][VERBOSE] Creating proxy for "local:commonapi.examples.E08CrcProtectionP04:v1_0:commonapi.examples.Attributes"
2022-09-12 16:13:42.156045 [info] Parsed vsomeip configuration in 1ms
2022-09-12 16:13:42.161035 [info] Using configuration file: "vsomeip-client.json".
2022-09-12 16:13:42.162321 [info] Configuration module loaded.
2022-09-12 16:13:42.163433 [info] Initializing vsomeip application "client-sample".
2022-09-12 16:13:42.168211 [info] Instantiating routing manager [Host].
2022-09-12 16:13:42.176024 [info] create_routing_root: Local routing @ /tmp/vsomeip-0
2022-09-12 16:13:42.181899 [info] Service Discovery enabled. Trying to load module.
2022-09-12 16:13:42.200256 [info] Service Discovery module loaded.
2022-09-12 16:13:42.202122 [info] E2E protection enabled.
2022-09-12 16:13:42.209612 [info] E2E module loaded.
2022-09-12 16:13:42.211254 [info] vsomeip tracing not enabled. . vsomeip service discovery tracing not enabled. 
2022-09-12 16:13:42.212131 [info] Application(client-sample, 1343) is initialized (11, 100).
2022-09-12 16:13:42.215525 [info] Starting vsomeip application "client-sample" (1343) using 2 threads I/O nice 255
2022-09-12 16:13:42.217985 [info] REGISTER EVENT(1343): [1234.0001.80e9:is_provider=false]
2022-09-12 16:13:42.218794 [info] main dispatch thread id from application: 1343 (client-sample) is: 7fe949c1c640 TID: 2809828
2022-09-12 16:13:42.219878 [info] shutdown thread id from application: 1343 (client-sample) is: 7fe94941b640 TID: 2809829
2022-09-12 16:13:42.219629 [info] REGISTER EVENT(1343): [1234.0001.80ea:is_provider=false]
2022-09-12 16:13:42.222334 [info] Watchdog is disabled!
2022-09-12 16:13:42.223218 [info] REQUEST(1343): [1234.0001:1.4294967295]
2022-09-12 16:13:42.225445 [info] io thread id from application: 1343 (client-sample) is: 7fe94a41d640 TID: 2809827
2022-09-12 16:13:42.225453 [info] io thread id from application: 1343 (client-sample) is: 7fe93bfff640 TID: 2809831
2022-09-12 16:13:42.228753 [info] create_local_server: Listening @ /tmp/vsomeip-1343
2022-09-12 16:13:42.230708 [info] Network interface "lxcbr0" state changed: up
2022-09-12 16:13:42.233703 [info] Route "224.0.0.0/4 if: lxcbr0 gw: n/a" state changed: up
2022-09-12 16:13:42.229153 [info] vSomeIP 3.2.19 | (default)
2022-09-12 16:13:42.234239 [info] SUBSCRIBE(1343): [1234.0001.0001:80e9:1]
2022-09-12 16:13:42.237934 [info] SUBSCRIBE(1343): [1234.0001.0002:80ea:1]
2022-09-12 16:13:42.238613 [info] udp_server_endpoint_impl: SO_RCVBUF is: 212992
Waiting for service to become available.
2022-09-12 16:13:42.240588 [debug] Joining to multicast group 224.244.224.245 from 10.0.3.1
2022-09-12 16:13:42.242950 [info] SOME/IP routing ready.
Got valid response for aOK Async getter
Got valid response for aERROR Async getter
2022-09-12 16:13:43.714948 [info] E2E protection: CRC check failed for service: 1234 method: 80ea
Received change message on aOK
	CommonCRCLength: 24
	CommonCRCCounter: 41
	CommonCRCID: 16777471
	CommonCRC: 758244104
	value1: 2
	value2: 3
Subscription (Changed Event) of aOK attribute returned CallStatus==INVALID_VALUE
2022-09-12 16:13:47.341330 [info] E2E protection: CRC check failed for service: 1234 method: 80ea
Received change message on aOK:
	CommonCRCLength: 24
	CommonCRCCounter: 42
	CommonCRCID: 16777471
	CommonCRC: 3547810833
	value1: 3
	value2: 4
Subscription (Changed Event) of aOK attribute returned CallStatus==INVALID_VALUE
2022-09-12 16:13:52.349024 [info] E2E protection: CRC check failed for service: 1234 method: 80ea
Received change message on aOK:
	CommonCRCLength: 24
	CommonCRCCounter: 43
	CommonCRCID: 16777471
	CommonCRC: 1160927003
	value1: 4
	value2: 5
Subscription (Changed Event) of aOK attribute returned CallStatus==INVALID_VALUE
```

On the Client side we can see, that even though both subscriptions receive a valid response, on every change event notification, it is possible to obtain the *aOK* attribute values, including the automatically set CRC values, but the notification for the *aERROR* attribute always returns an *INVALID_VALUE*, due to the CRC check failed for this method.
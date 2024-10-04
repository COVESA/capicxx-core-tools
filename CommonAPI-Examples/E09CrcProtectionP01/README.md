# Example 9: CRC Protection Using Profile P01

This example was created to demonstrate how the e2e implementation can be used on the CommonAPI layer with profile P01, also referenced as CRC8. Please note that this example can only be correctly used if the Service and the Client are running on distinct machines/ECUs.
First consider the Franca IDL specification of example 9 **E09CrcProtectionP01.fidl**:

```java
package commonapi.examples

interface E09CrcProtectionP01 {
    version { major 1 minor 0 }

    attribute CommonTypes.aStruct aOK
    attribute CommonTypes.aStruct aERROR
}

typeCollection CommonTypes {
    version { major 1 minor 0 }
    
    struct aStruct {
          UInt8 CRC8 
          UInt8 CRC8Counter
          UInt8 value1
          UInt8 value2
    }
}
```

The following CRC related fields in the *aStruct* (*CRC8*, *CRC8Counter*) are automatically computed and setted using the required e2e definitions on the .json file.
In this example we will demonstrate how distinct e2e definitions on the configuration file for the service and the client for the *aERROR* attribute lead to a *CallStatus::INVALID_VALUE* on a changed event notification, after a subscription.

Let's start by the configuration file used for the service **vsomeip-service.json**:

```json
{
    "unicast" : "172.17.0.2",
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
                "profile" : "CRC8",
                "variant" : "protector",
                "crc_offset" : "0",
                "data_id_mode" : "3",
                "data_length" : "24",
                "data_id" : "0x1FF"
            },
            {
                "service_id" : "0x1234",
                "event_id" : "0x80ea",
                "profile" : "CRC8",
                "variant" : "protector",
                "crc_offset" : "0",
                "data_id_mode" : "3",
                "data_length" : "24",
                "data_id" : "0x1FF"
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
    "unicast" : "172.17.0.3",
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
                "profile" : "CRC8",
                "variant" : "checker",
                "crc_offset" : "0",
                "data_id_mode" : "3",
                "data_length" : "24",
                "data_id" : "0x1FF"
            },
            {
                "service_id" : "0x1234",
                "event_id" : "0x80ea",
                "profile" : "CRC8",
                "variant" : "checker",
                "crc_offset" : "0",
                "data_id_mode" : "3",
                "data_length" : "24",
                "data_id" : "0x000"
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

Between both it is possible to see, in the e2e definitions, that the *data_id* for the *aERROR* attribute (*event_id 0x80ea*) has distinct values between the service and the client, *0x1FF* vs *0x000*, while the definitions for the *aOK* attribute (*event_id 0x80e9*) match.
Please note that the first definition, *unicast* will need to be adapted to your specific setup.

Now let's have a look to the CommonAPI code on the service side, that is splitted between the following 3 files:

* **E09CrcProtectionP01StubImpl.hpp**:
```cpp
#ifndef E09CRCPROTECTIONP01STUBIMPL_HPP_
#define E09CRCPROTECTIONP01STUBIMPL_HPP_

#include <CommonAPI/CommonAPI.hpp>
#include <v1/commonapi/examples/E09CrcProtectionP01StubDefault.hpp>

class E09CrcProtectionP01StubImpl: public v1_0::commonapi::examples::E09CrcProtectionP01StubDefault {

public:
    E09CrcProtectionP01StubImpl();
    virtual ~E09CrcProtectionP01StubImpl();
    virtual void incCounter();

private:
    int cnt;
};

#endif // E09CRCPROTECTIONP01STUBIMPL_HPP_
```

* **E09CrcProtectionP01StubImpl.cpp**:
```cpp
#include "E09CrcProtectionP01StubImpl.hpp"

using namespace v1::commonapi::examples;

E09CrcProtectionP01StubImpl::E09CrcProtectionP01StubImpl() {
    cnt = 0;
}

E09CrcProtectionP01StubImpl::~E09CrcProtectionP01StubImpl() {
}

void E09CrcProtectionP01StubImpl::incCounter() {
    cnt++;

    CommonTypes::aStruct aOK, aERROR;
    aOK.setValue1((uint8_t)cnt);
    aOK.setValue2((uint8_t)cnt+1);
    setAOKAttribute(aOK);

    aERROR.setValue1((uint8_t)cnt);
    aERROR.setValue2((uint8_t)cnt+2);
    setAERRORAttribute(aERROR);
    std::cout <<  "New counter value = " << cnt << "!" << std::endl;
}
```

* **E09CrcProtectionP01Service.cpp**:
```cpp
#include <iostream>
#include <thread>

#include <CommonAPI/CommonAPI.hpp>
#include "E09CrcProtectionP01StubImpl.hpp"

int main() {
    CommonAPI::Runtime::setProperty("LogContext", "E09S");
    CommonAPI::Runtime::setProperty("LogApplication", "E09S");
    CommonAPI::Runtime::setProperty("LibraryBase", "E09CrcProtectionP01");

    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();

    std::string domain = "local";
    std::string instance = "commonapi.examples.Attributes";
    std::string connection = "service-sample";

    std::shared_ptr<E09CrcProtectionP01StubImpl> myService = std::make_shared<E09CrcProtectionP01StubImpl>();
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

Now, we move into the implementation for the client side in **E09CrcProtectionP01Client.cpp**:
```cpp
#include <iostream>
#include <thread>

#ifndef _WIN32
#include <unistd.h>
#endif

#include <CommonAPI/CommonAPI.hpp>
#include <CommonAPI/Extensions/AttributeCacheExtension.hpp>
#include <v1/commonapi/examples/E09CrcProtectionP01Proxy.hpp>

using namespace v1::commonapi::examples;

int main() {
    CommonAPI::Runtime::setProperty("LogContext", "E09C");
    CommonAPI::Runtime::setProperty("LogApplication", "E09C");
    CommonAPI::Runtime::setProperty("LibraryBase", "E09CrcProtectionP01");

    std::shared_ptr < CommonAPI::Runtime > runtime = CommonAPI::Runtime::get();

    std::string domain = "local";
    std::string instance = "commonapi.examples.Attributes"; 
    std::string connection = "client-sample";

    auto myProxy = runtime->buildProxyWithDefaultAttributeExtension<E09CrcProtectionP01Proxy, CommonAPI::Extensions::AttributeCacheExtension>(domain, instance, connection);

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
                      << "\tCRC: " << static_cast<int>(val.getCRC8()) << std::endl
                      << "\tID Nibble: " << static_cast<int>(val.getCRC8Counter() >> 4) << std::endl
                      << "\tCounter: " << static_cast<int>(val.getCRC8Counter() & 0x0F) << std::endl
	                  << "\tvalue1: " << static_cast<int>(val.getValue1()) << std::endl
  	                  << "\tvalue2: " << static_cast<int>(val.getValue2()) << std::endl;
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
                      << "\tCRC: " << static_cast<int>(val.getCRC8()) << std::endl
                      << "\tCounter: " << static_cast<int>(val.getCRC8Counter()) << std::endl
	                  << "\tvalue1: " << static_cast<int>(val.getValue1()) << std::endl
  	                  << "\tvalue2: " << static_cast<int>(val.getValue2()) << std::endl;
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
bmw:/mnt/E09CrcProtectionP01P01$ VSOMEIP_CONFIGURATION=vsomeip-service.json COMMONAPI_CONFIG=$PWD/commonapi4someip.ini LD_LIBRARY_PATH=<path-to-install-folder>/lib:$PWD/build   build/E09CrcProtectionP01Service
[CAPI][INFO] Loading configuration file '/workspaces/network-workspace/src/capi/capicxx-core-tools/CommonAPI-Examples/E09CrcProtectionP01P01/commonapi4someip.ini'
[CAPI][INFO] Using default binding 'someip'
[CAPI][INFO] Using default shared library folder '/usr/local/lib/commonapi'
[CAPI][DEBUG] Loading library for local:commonapi.examples.E09CrcProtectionP01:v1_0:commonapi.examples.Attributes stub.
[CAPI][DEBUG] Added address mapping: local:commonapi.examples.E09CrcProtectionP01:v1_0:commonapi.examples.Attributes <--> [1234.0001(1.0)]
[CAPI][VERBOSE] Registering function for creating "commonapi.examples.E09CrcProtectionP01:v1_0" proxy.
[CAPI][INFO] Registering function for creating "commonapi.examples.E09CrcProtectionP01:v1_0" stub adapter.
[CAPI][DEBUG] Loading interface library "libE09CrcProtectionP01-someip.so" succeeded.
[CAPI][INFO] Registering stub for "local:commonapi.examples.E09CrcProtectionP01:v1_0:commonapi.examples.Attributes"
2023-09-22 15:48:43.113251 [info] Using configuration file: "src/capi/capicxx-core-tools/CommonAPI-Examples/E09CrcProtectionP01P01/vsomeip-service.json".
2023-09-22 15:48:43.113572 [info] Parsed vsomeip configuration in 0ms
2023-09-22 15:48:43.113633 [info] Configuration module loaded.
2023-09-22 15:48:43.113678 [info] Initializing vsomeip (3.3.8) application "service-sample".
2023-09-22 15:48:43.113854 [info] Instantiating routing manager [Host].
2023-09-22 15:48:43.114788 [info] create_routing_root: Routing root @ /tmp/vsomeip-0
2023-09-22 15:48:43.115443 [info] Service Discovery enabled. Trying to load module.
2023-09-22 15:48:43.116821 [info] Service Discovery module loaded.
2023-09-22 15:48:43.117022 [info] E2E protection enabled.
2023-09-22 15:48:43.117642 [info] E2E module loaded.
2023-09-22 15:48:43.117794 [info] Application(service-sample, 1277) is initialized (11, 100).
2023-09-22 15:48:43.118421 [info] Starting vsomeip application "service-sample" (1277) using 2 threads I/O nice 255
2023-09-22 15:48:43.119186 [info] Client [1277] routes unicast:172.17.0.2, netmask:255.255.255.0
2023-09-22 15:48:43.119539 [info] offer_event: Event [1234.0001.80e9] uses configured cycle time 0ms
2023-09-22 15:48:43.119361 [info] main dispatch thread id from application: 1277 (service-sample) is: 7f73b1103700 TID: 540759
2023-09-22 15:48:43.119188 [info] shutdown thread id from application: 1277 (service-sample) is: 7f73b0902700 TID: 540760
2023-09-22 15:48:43.119740 [info] REGISTER EVENT(1277): [1234.0001.80e9:is_provider=true]
2023-09-22 15:48:43.120162 [info] Watchdog is disabled!
2023-09-22 15:48:43.120264 [info] offer_event: Event [1234.0001.80ea] uses configured cycle time 0ms
2023-09-22 15:48:43.120332 [info] REGISTER EVENT(1277): [1234.0001.80ea:is_provider=true]
2023-09-22 15:48:43.120491 [info] io thread id from application: 1277 (service-sample) is: 7f73b1904700 TID: 540758
2023-09-22 15:48:43.120655 [info] OFFER(1277): [1234.0001:1.0] (true)
2023-09-22 15:48:43.120500 [info] io thread id from application: 1277 (service-sample) is: 7f73a37fe700 TID: 540762
2023-09-22 15:48:43.120996 [info] vSomeIP 3.3.8 | (default)
2023-09-22 15:48:43.121477 [info] create_local_server: Listening @ /tmp/vsomeip-1277
2023-09-22 15:48:43.121467 [info] Network interface "eth0" state changed: up
Successfully Registered Service!
New counter value = 1!
Waiting for calls... (Abort with CTRL+C)
2023-09-22 15:48:43.122054 [info] Route "default route (0.0.0.0/0) if: eth0 gw: 172.17.0.1" state changed: up
2023-09-22 15:48:43.122566 [info] udp_server_endpoint_impl: SO_RCVBUF is: 212992 (1703936) local port:30490
2023-09-22 15:48:43.122688 [debug] Joining to multicast group 224.244.224.245 from 172.17.0.2
2023-09-22 15:48:43.123310 [info] udp_server_endpoint_impl<multicast>: SO_RCVBUF is: 212992 (1703936) local port:30490
2023-09-22 15:48:43.123332 [info] SOME/IP routing ready.
2023-09-22 15:48:45.509821 [info] REMOTE SUBSCRIBE(0000): [1234.0001.0001] from 172.17.0.3:46209 reliable was accepted
2023-09-22 15:48:45.511029 [info] REMOTE SUBSCRIBE(0000): [1234.0001.0002] from 172.17.0.3:46209 reliable was accepted
New counter value = 2!
Waiting for calls... (Abort with CTRL+C)
2023-09-22 15:48:53.123233 [info] vSomeIP 3.3.8 | (default)
New counter value = 3!
Waiting for calls... (Abort with CTRL+C)
New counter value = 4!
Waiting for calls... (Abort with CTRL+C)
2023-09-22 15:49:03.124752 [info] vSomeIP 3.3.8 | (default)
New counter value = 5!
Waiting for calls... (Abort with CTRL+C)
New counter value = 6!
Waiting for calls... (Abort with CTRL+C)
2023-09-22 15:49:13.126490 [info] vSomeIP 3.3.8 | (default)
New counter value = 7!
Waiting for calls... (Abort with CTRL+C)
New counter value = 8!
Waiting for calls... (Abort with CTRL+C)
2023-09-22 15:49:23.128458 [info] vSomeIP 3.3.8 | (default)
New counter value = 9!
Waiting for calls... (Abort with CTRL+C)
New counter value = 10!
Waiting for calls... (Abort with CTRL+C)
2023-09-22 15:49:33.130217 [info] vSomeIP 3.3.8 | (default)
New counter value = 11!
Waiting for calls... (Abort with CTRL+C)
New counter value = 12!
Waiting for calls... (Abort with CTRL+C)
2023-09-22 15:49:43.131073 [info] vSomeIP 3.3.8 | (default)
New counter value = 13!
Waiting for calls... (Abort with CTRL+C)
New counter value = 14!
Waiting for calls... (Abort with CTRL+C)
2023-09-22 15:49:53.131988 [info] vSomeIP 3.3.8 | (default)
New counter value = 15!
Waiting for calls... (Abort with CTRL+C)
New counter value = 16!
Waiting for calls... (Abort with CTRL+C)
```

```bash
bmw:~/Documents/commonapi-core-generator/CommonAPI-Examples/E09CrcProtectionP01P01$ VSOMEIP_CONFIGURATION=vsomeip-client.json COMMONAPI_CONFIG=$PWD/commonapi4someip.ini LD_LIBRARY_PATH=../../../install_folder/lib:$PWD/build   build/E09CrcProtectionP01Client
[CAPI][INFO] Loading configuration file '/workspaces/network-workspace/src/capi/capicxx-core-tools/CommonAPI-Examples/E09CrcProtectionP01P01/commonapi4someip.ini'
[CAPI][INFO] Using default binding 'someip'
[CAPI][INFO] Using default shared library folder '/usr/local/lib/commonapi'
[CAPI][DEBUG] Loading library for local:commonapi.examples.E09CrcProtectionP01:v1_0:commonapi.examples.Attributes proxy.
[CAPI][DEBUG] Added address mapping: local:commonapi.examples.E09CrcProtectionP01:v1_0:commonapi.examples.Attributes <--> [1234.0001(1.0)]
[CAPI][VERBOSE] Registering function for creating "commonapi.examples.E09CrcProtectionP01:v1_0" proxy.
[CAPI][INFO] Registering function for creating "commonapi.examples.E09CrcProtectionP01:v1_0" stub adapter.
[CAPI][DEBUG] Loading interface library "libE09CrcProtectionP01-someip.so" succeeded.
[CAPI][VERBOSE] Creating proxy for "local:commonapi.examples.E09CrcProtectionP01:v1_0:commonapi.examples.Attributes"
2023-09-22 15:48:38.807363 [info] Using configuration file: "src/capi/capicxx-core-tools/CommonAPI-Examples/E09CrcProtectionP01P01/vsomeip-client.json".
2023-09-22 15:48:38.808003 [info] Parsed vsomeip configuration in 0ms
2023-09-22 15:48:38.808125 [info] Configuration module loaded.
2023-09-22 15:48:38.808232 [info] Initializing vsomeip (3.3.8) application "client-sample".
2023-09-22 15:48:38.808479 [info] Instantiating routing manager [Host].
2023-09-22 15:48:38.810178 [info] create_routing_root: Routing root @ /tmp/vsomeip-0
2023-09-22 15:48:38.811300 [info] Service Discovery enabled. Trying to load module.
2023-09-22 15:48:38.813222 [info] Service Discovery module loaded.
2023-09-22 15:48:38.813607 [info] E2E protection enabled.
2023-09-22 15:48:38.814441 [info] E2E module loaded.
2023-09-22 15:48:38.814699 [info] Application(client-sample, 1343) is initialized (11, 100).
2023-09-22 15:48:38.815996 [info] Starting vsomeip application "client-sample" (1343) using 2 threads I/O nice 255
2023-09-22 15:48:38.816633 [info] REGISTER EVENT(1343): [1234.0001.80e9:is_provider=false]
2023-09-22 15:48:38.816821 [info] Client [1343] routes unicast:172.17.0.3, netmask:255.255.255.0
2023-09-22 15:48:38.816940 [info] REGISTER EVENT(1343): [1234.0001.80ea:is_provider=false]
2023-09-22 15:48:38.816814 [info] shutdown thread id from application: 1343 (client-sample) is: 7f4283fff700 TID: 220990
2023-09-22 15:48:38.816712 [info] main dispatch thread id from application: 1343 (client-sample) is: 7f4288d84700 TID: 220989
2023-09-22 15:48:38.817224 [info] REQUEST(1343): [1234.0001:1.4294967295]
2023-09-22 15:48:38.817960 [info] Watchdog is disabled!
2023-09-22 15:48:38.818373 [info] io thread id from application: 1343 (client-sample) is: 7f4289585700 TID: 220988
2023-09-22 15:48:38.818403 [info] io thread id from application: 1343 (client-sample) is: 7f4282ffd700 TID: 220992
2023-09-22 15:48:38.818806 [info] create_local_server: Listening @ /tmp/vsomeip-1343
2023-09-22 15:48:38.819053 [info] vSomeIP 3.3.8 | (default)
2023-09-22 15:48:38.819421 [info] Network interface "eth0" state changed: up
2023-09-22 15:48:38.819794 [info] Route "default route (0.0.0.0/0) if: eth0 gw: 172.17.0.1" state changed: up
2023-09-22 15:48:38.820132 [info] SUBSCRIBE(1343): [1234.0001.0001:80e9:1]
2023-09-22 15:48:38.820200 [info] udp_server_endpoint_impl: SO_RCVBUF is: 212992 (1703936) local port:30490
2023-09-22 15:48:38.820299 [info] notify_one_unlocked: Notifying 1234.1.80e9 to client 1343 failed. Event payload not set!
2023-09-22 15:48:38.820428 [debug] Joining to multicast group 224.244.224.245 from 172.17.0.3
2023-09-22 15:48:38.820599 [info] SUBSCRIBE(1343): [1234.0001.0002:80ea:1]
2023-09-22 15:48:38.820633 [info] SOME/IP routing ready.
2023-09-22 15:48:38.820712 [info] notify_one_unlocked: Notifying 1234.1.80ea to client 1343 failed. Event payload not set!
Waiting for service to become available.
2023-09-22 15:48:38.821086 [info] udp_server_endpoint_impl<multicast>: SO_RCVBUF is: 212992 (1703936) local port:30490
2023-09-22 15:48:45.475193 [info] endpoint_manager_impl::create_remote_client: 172.17.0.2:30499 reliable: 1 using local port: 0
Got valid response for aOK Async getter
Got valid response for aERROR Async getter
2023-09-22 15:48:45.561813 [info] E2E protection: CRC8 does not match: calculated CRC: 70 received CRC: 9f
2023-09-22 15:48:45.562050 [info] E2E protection: CRC check failed for service: 1234 method: 80ea
Received change message on aOK:
        CRC: 130
        ID Nibble: 1
        Counter: 2
        value1: 1
        value2: 2
Subscription (Changed Event) of aERROR attribute returned CallStatus==INVALID_VALUE
2023-09-22 15:48:48.819654 [info] vSomeIP 3.3.8 | (default)
2023-09-22 15:48:48.176039 [info] E2E protection: CRC8 does not match: calculated CRC: 78 received CRC: 97
2023-09-22 15:48:48.176486 [info] E2E protection: CRC check failed for service: 1234 method: 80ea
Received change message on aOK:
        CRC: 196
        ID Nibble: 1
        Counter: 3
        value1: 2
        value2: 3
Subscription (Changed Event) of aERROR attribute returned CallStatus==INVALID_VALUE
2023-09-22 15:48:53.178855 [info] E2E protection: CRC8 does not match: calculated CRC: a3 received CRC: 4c
2023-09-22 15:48:53.179734 [info] E2E protection: CRC check failed for service: 1234 method: 80ea
Received change message on aOK:
        CRC: 81
        ID Nibble: 1
        Counter: 4
        value1: 3
        value2: 4
Subscription (Changed Event) of aERROR attribute returned CallStatus==INVALID_VALUE
2023-09-22 15:48:58.820747 [info] vSomeIP 3.3.8 | (default)
2023-09-22 15:48:58.179910 [info] E2E protection: CRC8 does not match: calculated CRC: f2 received CRC: 1d
Received change message on aOK:
        CRC: 58
        ID Nibble: 1
        Counter: 5
        value1: 4
        value2: 5
2023-09-22 15:48:58.180382 [info] E2E protection: CRC check failed for service: 1234 method: 80ea
Subscription (Changed Event) of aERROR attribute returned CallStatus==INVALID_VALUE
.
.
.
.
2023-09-22 15:49:33.197830 [info] E2E protection: CRC8 does not match: calculated CRC: 1d received CRC: f2
Received change message on aOK:
        CRC: 239 
        ID Nibble: 1
        Counter: 12
        value1: 11
        value2: 12
2023-09-22 15:49:33.198746 [info] E2E protection: CRC check failed for service: 1234 method: 80ea
Subscription (Changed Event) of aERROR attribute returned CallStatus==INVALID_VALUE
2023-09-22 15:49:38.825198 [info] vSomeIP 3.3.8 | (default)
2023-09-22 15:49:38.198628 [info] E2E protection: CRC8 does not match: calculated CRC: 4c received CRC: a3
Received change message on aOK:
        CRC: 132
        ID Nibble: 1
        Counter: 13
        value1: 12
        value2: 13
2023-09-22 15:49:38.199328 [info] E2E protection: CRC check failed for service: 1234 method: 80ea
Subscription (Changed Event) of aERROR attribute returned CallStatus==INVALID_VALUE
2023-09-22 15:49:43.203916 [info] E2E protection: CRC8 does not match: calculated CRC: 91 received CRC: 7e
Received change message on aOK:
        CRC: 99
        ID Nibble: 1
        Counter: 14
        value1: 13
        value2: 14
2023-09-22 15:49:43.205027 [info] E2E protection: CRC check failed for service: 1234 method: 80ea
Subscription (Changed Event) of aERROR attribute returned CallStatus==INVALID_VALUE
2023-09-22 15:49:48.826064 [info] vSomeIP 3.3.8 | (default)
2023-09-22 15:49:48.204628 [info] E2E protection: CRC8 does not match: calculated CRC: 3a received CRC: d5
2023-09-22 15:49:48.205570 [info] E2E protection: CRC check failed for service: 1234 method: 80ea
Received change message on aOK:
        CRC: 163
        ID Nibble: 1
        Counter: 0
        value1: 14
        value2: 15
Subscription (Changed Event) of aERROR attribute returned CallStatus==INVALID_VALUE
2023-09-22 15:49:53.209003 [info] E2E protection: CRC8 does not match: calculated CRC: e4 received CRC: b
2023-09-22 15:49:53.210144 [info] E2E protection: CRC check failed for service: 1234 method: 80ea
Received change message on aOK:
        CRC: 22
        ID Nibble: 1
        Counter: 1
        value1: 15
        value2: 16
Subscription (Changed Event) of aERROR attribute returned CallStatus==INVALID_VALUE
2023-09-22 15:49:58.827399 [info] vSomeIP 3.3.8 | (default)
2023-09-22 15:49:58.209297 [info] E2E protection: CRC8 does not match: calculated CRC: 58 received CRC: b7
2023-09-22 15:49:58.210355 [info] E2E protection: CRC check failed for service: 1234 method: 80ea
Received change message on aOK:
        CRC: 144
        ID Nibble: 1
        Counter: 2
        value1: 16
        value2: 17
Subscription (Changed Event) of aERROR attribute returned CallStatus==INVALID_VALUE
```

On the Client side we can see, that even though both subscriptions receive a valid response, on every change event notification, it is possible to obtain the *aOK* attribute values, including the automatically set CRC values, but the notification for the *aERROR* attribute always returns an *INVALID_VALUE*, due to the CRC check failed for this method.
We can also check from the client logs that the counter rolls over after it reaches 14 and, for this specific mode ("data_id_mode" : "3"), the low nibble of the high byte from the Data ID field.
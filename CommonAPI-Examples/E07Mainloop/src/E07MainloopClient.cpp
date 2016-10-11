/* Copyright (C) 2015 BMW Group
 * Author: Lutz Bichler (lutz.bichler@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iostream>

#ifndef WIN32
#include <unistd.h>
#endif

#include <sstream>

#include <glib.h>
#include <gio/gio.h>

#include <CommonAPI/CommonAPI.hpp>
#include <v1/commonapi/examples/E07MainloopProxy.hpp>

using namespace v1_0::commonapi::examples;

GIOChannel* channel;

std::future<CommonAPI::CallStatus> gFutureCallStatus;
std::future<CommonAPI::CallStatus> gFutureCallStatusISIA;
std::future<CommonAPI::CallStatus> gFutureCallStatusGetAttrX;

int32_t gValueForX = 428394;

void myAttrXCallback(const CommonAPI::CallStatus& callStatus, const int32_t& val) {
    std::cout << "Receive callback for Attribute x: " << val << std::endl;
}

void mySayHelloCallback(const CommonAPI::CallStatus& _callStatus, const std::string& _returnMessage) {

    if (_callStatus != CommonAPI::CallStatus::SUCCESS) {
        std::cerr << "Remote call failed!\n";
        return;
    }
    std::cout << "Got message: '" << _returnMessage << "'\n";

}

gboolean callSetAttrX(void* proxy) {

    std::cout << "callSetAttrX called ..." << std::endl;

    E07MainloopProxy<>* myProxy = static_cast<E07MainloopProxy<>*>(proxy);
    myProxy->getXAttribute().setValueAsync(gValueForX , myAttrXCallback);

    return false;
}

gboolean callGetAttrX(void* proxy) {

    std::cout << "callGetAttrX called ..." << std::endl;

    E07MainloopProxy<>* myProxy = static_cast<E07MainloopProxy<>*>(proxy);
    myProxy->getXAttribute().getValueAsync(myAttrXCallback);

    return false;
}

gboolean callSayHello(void* proxy) {

    std::cout << "callSayHello called ..." << std::endl;

    static int number = 1;

    std::stringstream stream;
    stream << "World (" << number << ")";
    const std::string name = stream.str();

    E07MainloopProxy<>* myProxy = static_cast<E07MainloopProxy<>*>(proxy);
    gFutureCallStatus = myProxy->sayHelloAsync(name, mySayHelloCallback);

    number++;

    return true;
}

class GDispatchWrapper: public GSource {
 public:
    GDispatchWrapper(CommonAPI::DispatchSource* dispatchSource): dispatchSource_(dispatchSource) {}
    CommonAPI::DispatchSource* dispatchSource_;
};

gboolean dispatchPrepare ( GSource* source, gint* timeout ) {

    bool result = false;
    int64_t eventTimeout;

    result = static_cast<GDispatchWrapper*>(source)->dispatchSource_->prepare(eventTimeout);

    *timeout = eventTimeout;

    return result;
}

gboolean dispatchCheck ( GSource* source ) {

    return static_cast<GDispatchWrapper*>(source)->dispatchSource_->check();
}

gboolean dispatchExecute ( GSource* source, GSourceFunc callback, gpointer userData ) {

    static_cast<GDispatchWrapper*>(source)->dispatchSource_->dispatch();
    return true;
}

static GSourceFuncs standardGLibSourceCallbackFuncs = {
    dispatchPrepare,
    dispatchCheck,
    dispatchExecute,
    NULL
};

gboolean gWatchDispatcher ( GIOChannel *source, GIOCondition condition, gpointer userData ) {

    CommonAPI::Watch* watch = static_cast<CommonAPI::Watch*>(userData);

#ifdef WIN32
    condition = static_cast<GIOCondition>(POLLIN);
#endif

    watch->dispatch(condition);
    return true;
}

gboolean gTimeoutDispatcher ( void* userData ) {

    return static_cast<CommonAPI::DispatchSource*>(userData)->dispatch();
}

void watchAddedCallback ( CommonAPI::Watch* watch, const CommonAPI::DispatchPriority dispatchPriority ) {
    const pollfd& fileDesc = watch->getAssociatedFileDescriptor();

#ifdef WIN32
    channel = g_io_channel_win32_new_socket(fileDesc.fd);
    GSource* gWatch = g_io_create_watch(channel, GIOCondition::G_IO_IN);
#else
    channel = g_io_channel_unix_new(fileDesc.fd); 
    GSource* gWatch = g_io_create_watch(channel, static_cast<GIOCondition>(fileDesc.events));
#endif

    g_source_set_callback(gWatch, reinterpret_cast<GSourceFunc>(&gWatchDispatcher), watch, NULL);

    const auto& dependentSources = watch->getDependentDispatchSources();
    for (auto dependentSourceIterator = dependentSources.begin();
            dependentSourceIterator != dependentSources.end();
            dependentSourceIterator++) {
        GSource* gDispatchSource = g_source_new(&standardGLibSourceCallbackFuncs, sizeof(GDispatchWrapper));
        static_cast<GDispatchWrapper*>(gDispatchSource)->dispatchSource_ = *dependentSourceIterator;

        g_source_add_child_source(gWatch, gDispatchSource);

    }
    int source = g_source_attach(gWatch, NULL);
}

void watchRemovedCallback ( CommonAPI::Watch* watch ) {

    g_source_remove_by_user_data(watch);

    if(channel) {
        g_io_channel_unref(channel);
        channel = NULL;
    }
}

int main() {
    CommonAPI::Runtime::setProperty("LogContext", "E07C");
    CommonAPI::Runtime::setProperty("LogApplication", "E07C");
    CommonAPI::Runtime::setProperty("LibraryBase", "E07Mainloop");

    std::shared_ptr < CommonAPI::Runtime > runtime = CommonAPI::Runtime::get();

    std::string domain = "local";
    std::string instance = "commonapi.examples.Mainloop";
    std::string connection = "client-sample";

    std::shared_ptr<CommonAPI::MainLoopContext> mainloopContext = std::make_shared<CommonAPI::MainLoopContext>(connection);

    std::function<void(CommonAPI::Watch*, const CommonAPI::DispatchPriority)> f_watchAddedCallback = watchAddedCallback;
    std::function<void(CommonAPI::Watch*)> f_watchRemovedCallback = watchRemovedCallback;
    mainloopContext->subscribeForWatches(f_watchAddedCallback, f_watchRemovedCallback);

    std::shared_ptr<E07MainloopProxy<>> myProxy = runtime->buildProxy<E07MainloopProxy>(domain,
            instance, mainloopContext);

    std::cout << "Checking availability" << std::flush;
    static 
        #ifndef WIN32
            constexpr
        #endif
    bool mayBlock = false;
        
    int count = 0;
    while (!myProxy->isAvailable()) {
        if (count % 10 == 0)
            std::cout << "." << std::flush;
        g_main_context_iteration(NULL, mayBlock);
        usleep(50000);
    }
    std::cout << "done." << std::endl;

    GMainLoop* mainloop = NULL;
    mainloop = g_main_loop_new(NULL, FALSE);

    void *proxyPtr = (void*)myProxy.get();
    g_timeout_add(100, callSayHello, proxyPtr);
    g_timeout_add(5000, callGetAttrX, proxyPtr);
    g_timeout_add(9000, callSetAttrX, proxyPtr);

    g_main_loop_run (mainloop);
    g_main_loop_unref (mainloop);

    return 0;
}

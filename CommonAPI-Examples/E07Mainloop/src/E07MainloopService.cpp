/* Copyright (C) 2015 BMW Group
 * Author: Lutz Bichler (lutz.bichler@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iostream>
#include <thread>

#include <glib.h>
#include <gio/gio.h>

#include <CommonAPI/CommonAPI.hpp>
#include "E07MainloopStubImpl.hpp"

using namespace std;

GIOChannel* channel;

gboolean callIncCounter(void* service) {

    E07MainloopStubImpl* myService = static_cast<E07MainloopStubImpl*>(service);
    myService->incAttrX();

    return true;
}

gboolean callWaitingInfo(void* service) {
    std::cout << "Waiting for calls... (Abort with CTRL+C)" << std::endl;

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

#ifdef _WIN32
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

#ifdef _WIN32
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
    CommonAPI::Runtime::setProperty("LogContext", "E07S");
    CommonAPI::Runtime::setProperty("LogApplication", "E07S");
    CommonAPI::Runtime::setProperty("LibraryBase", "E07Mainloop");

    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();

    std::string domain = "local";
    std::string instance = "commonapi.examples.Mainloop";
    std::string connection = "service-sample";

    std::shared_ptr<CommonAPI::MainLoopContext> mainloopContext = std::make_shared<CommonAPI::MainLoopContext>(connection);

    std::function<void(CommonAPI::Watch*, const CommonAPI::DispatchPriority)> f_watchAddedCallback = watchAddedCallback;
    std::function<void(CommonAPI::Watch*)> f_watchRemovedCallback = watchRemovedCallback;
    mainloopContext->subscribeForWatches(f_watchAddedCallback, f_watchRemovedCallback);

    std::shared_ptr<E07MainloopStubImpl> myService = std::make_shared<E07MainloopStubImpl>();

    bool successfullyRegistered = runtime->registerService(domain, instance, myService, mainloopContext);

    while (!successfullyRegistered) {
        std::cout << "Register Service failed, trying again in 100 milliseconds..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        successfullyRegistered = runtime->registerService(domain, instance, myService, mainloopContext);
    }

    std::cout << "Successfully Registered Service!" << std::endl;

    GMainLoop* mainloop = NULL;
    mainloop = g_main_loop_new(NULL, FALSE);

    void *servicePtr = (void*)myService.get();
    g_timeout_add_seconds(10, callWaitingInfo, servicePtr);

    g_timeout_add(2500, callIncCounter, servicePtr);

    g_main_loop_run (mainloop);
    g_main_loop_unref (mainloop);

    return 0;
}

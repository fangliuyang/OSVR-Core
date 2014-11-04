/** @file
    @brief Implementation

    @date 2014

    @author
    Ryan Pavlik
    <ryan@sensics.com>;
    <http://sensics.com>
*/

// Copyright 2014 Sensics, Inc.
//
// All rights reserved.
//
// (Final version intended to be licensed under
// the Apache License, Version 2.0)

// Internal Includes
#include <ogvr/PluginKit/DeviceInterfaceC.h>
#include <ogvr/PluginKit/PluginRegistration.h>
#include "AsyncDeviceToken.h"
#include "SyncDeviceToken.h"
#include <ogvr/PluginKit/DeviceToken.h>
#include <ogvr/PluginKit/MessageType.h>
#include <ogvr/PluginKit/Connection.h>
#include <ogvr/Util/Verbosity.h>
#include "HandleNullContext.h"
#include "PluginSpecificRegistrationContext.h"

// Library/third-party includes
// - none

// Standard includes
#include <functional>

OGVR_PluginReturnCode ogvrDeviceSendData(OGVR_INOUT OGVR_DeviceToken dev,
                                         OGVR_IN OGVR_MessageType msg,
                                         const char *bytestream, size_t len) {
    OGVR_DEV_VERBOSE(
        "In ogvrDeviceSendData, trying to send a message of length " << len);
    OGVR_PLUGIN_HANDLE_NULL_CONTEXT("ogvrDeviceSendData device token", dev);
    OGVR_PLUGIN_HANDLE_NULL_CONTEXT("ogvrDeviceSendData message type", msg);
    ogvr::DeviceToken *device = static_cast<ogvr::DeviceToken *>(dev);
    ogvr::MessageType *msgType = static_cast<ogvr::MessageType *>(msg);
    device->sendData(msgType, bytestream, len);
    return OGVR_PLUGIN_SUCCESS;
}

OGVR_PluginReturnCode
ogvrDeviceRegisterMessageType(OGVR_INOUT OGVR_PluginRegContext ctx,
                              OGVR_IN_STRZ const char *name,
                              OGVR_OUT OGVR_MessageType *msgtype) {
    OGVR_PLUGIN_HANDLE_NULL_CONTEXT("ogvrDeviceRegisterMessageType", ctx);
    OGVR_DEV_VERBOSE("In ogvrDeviceRegisterMessageType for a message named "
                     << name);

    ogvr::PluginSpecificRegistrationContext *context =
        static_cast<ogvr::PluginSpecificRegistrationContext *>(ctx);
    // Extract the connection from the overall context
    ogvr::ConnectionPtr conn =
        ogvr::Connection::retrieveConnection(context->getParent());
    ogvr::MessageTypePtr ret = conn->registerMessageType(name);

    // Transfer ownership of the message type object to the plugin context.
    try {
        *msgtype = ogvr::plugin::registerObjectForDeletion(ctx, ret.release());
    } catch (std::exception &e) {
        std::cerr << "Error in ogvrDeviceRegisterMessageType: " << e.what()
                  << std::endl;
        return OGVR_PLUGIN_FAILURE;
    } catch (...) {
        return OGVR_PLUGIN_FAILURE;
    }
    return OGVR_PLUGIN_SUCCESS;
}

template <typename FactoryFunction>
inline static OGVR_PluginReturnCode
ogvrDeviceGenericInit(OGVR_PluginRegContext ctx, const char *name,
                      OGVR_DeviceToken *device, FactoryFunction f) {
    ogvr::PluginSpecificRegistrationContext *context =
        static_cast<ogvr::PluginSpecificRegistrationContext *>(ctx);
    // Compute the name by combining plugin name with the given name
    std::string qualifiedName = context->getName() + "/" + name;

    OGVR_DEV_VERBOSE("Qualified name: " << qualifiedName);

    ogvr::RegistrationContext &overallContext(context->getParent());
    // Extract the connection from the overall context
    ogvr::ConnectionPtr conn =
        ogvr::Connection::retrieveConnection(overallContext);
    if (!conn) {
        OGVR_DEV_VERBOSE(
            "ogvrDeviceGenericInit Got a null Connection pointer - "
            "this shouldn't happen!");
        return OGVR_PLUGIN_FAILURE;
    }
    ogvr::DeviceTokenPtr dev = f(qualifiedName, conn);
    if (!dev) {
        OGVR_DEV_VERBOSE("Device token factory returned a null "
                         "pointer - this shouldn't happen!");
        return OGVR_PLUGIN_FAILURE;
    }
    // Transfer ownership of the device token object to the plugin context.
    try {
        *device = ogvr::plugin::registerObjectForDeletion(ctx, dev.release());
    } catch (std::exception &e) {
        std::cerr << "Error in ogvrDeviceGenericInit: " << e.what()
                  << std::endl;
        return OGVR_PLUGIN_FAILURE;
    } catch (...) {
        return OGVR_PLUGIN_FAILURE;
    }
    return OGVR_PLUGIN_SUCCESS;
}

OGVR_PluginReturnCode ogvrDeviceSyncInit(OGVR_INOUT OGVR_PluginRegContext ctx,
                                         OGVR_IN_STRZ const char *name,
                                         OGVR_OUT OGVR_DeviceToken *device) {
    OGVR_PLUGIN_HANDLE_NULL_CONTEXT("ogvrDeviceSyncInit", ctx);
    OGVR_DEV_VERBOSE("In ogvrDeviceSyncInit for a device named " << name);
    return ogvrDeviceGenericInit(ctx, name, device,
                                 ogvr::DeviceToken::createSyncDevice);
}

OGVR_PluginReturnCode
ogvrDeviceSyncRegisterUpdateCallback(OGVR_INOUT OGVR_DeviceToken device,
                                     OGVR_IN OGVR_SyncDeviceUpdateCallback
                                         updateCallback,
                                     OGVR_IN_OPT void *userData) {
    OGVR_DEV_VERBOSE("In ogvrDeviceSyncRegisterUpdateCallback");
    OGVR_PLUGIN_HANDLE_NULL_CONTEXT(
        "ogvrDeviceSyncRegisterUpdateCallback device token", device);
    ogvr::SyncDeviceToken *syncdev =
        static_cast<ogvr::DeviceToken *>(device)->asSyncDevice();
    if (!syncdev) {
        OGVR_DEV_VERBOSE("This isn't a synchronous device token!");
        return OGVR_PLUGIN_FAILURE;
    }
    syncdev->setUpdateCallback(std::bind(updateCallback, userData));
    return OGVR_PLUGIN_SUCCESS;
}

OGVR_PluginReturnCode ogvrDeviceAsyncInit(OGVR_INOUT OGVR_PluginRegContext ctx,
                                          OGVR_IN_STRZ const char *name,
                                          OGVR_OUT OGVR_DeviceToken *device) {
    OGVR_PLUGIN_HANDLE_NULL_CONTEXT("ogvrDeviceAsyncInit", ctx);
    OGVR_DEV_VERBOSE("In ogvrDeviceAsyncInit for a device named " << name);
    return ogvrDeviceGenericInit(ctx, name, device,
                                 ogvr::DeviceToken::createAsyncDevice);
}

OGVR_PluginReturnCode
ogvrDeviceAsyncStartWaitLoop(OGVR_INOUT OGVR_DeviceToken device,
                             OGVR_IN OGVR_AsyncDeviceWaitCallback waitCallback,
                             OGVR_IN_OPT void *userData) {
    OGVR_DEV_VERBOSE("In ogvrDeviceAsyncStartWaitLoop");
    OGVR_PLUGIN_HANDLE_NULL_CONTEXT("ogvrDeviceAsyncStartWaitLoop device token",
                                    device);
    ogvr::AsyncDeviceToken *asyncdev =
        static_cast<ogvr::DeviceToken *>(device)->asAsyncDevice();
    if (!asyncdev) {
        OGVR_DEV_VERBOSE("This isn't an asynchronous device token!");
        return OGVR_PLUGIN_FAILURE;
    }
    asyncdev->setWaitCallback(waitCallback, userData);
    return OGVR_PLUGIN_SUCCESS;
}
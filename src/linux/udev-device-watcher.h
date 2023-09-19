// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "../backend.h"
#include "../platform/device-watcher.h"
#include <rsutils/concurrency/concurrency.h>
#include "../callback-invocation.h"

#include <libudev.h>


namespace librealsense {


class udev_device_watcher : public librealsense::platform::device_watcher
{
    active_object<> _active_object;

    callbacks_heap _callback_inflight;
    platform::backend const * _backend;

    mutable std::mutex _mutex;
    platform::backend_device_group _devices_data;
    platform::device_changed_callback _callback;

    struct udev * _udev_ctx;
    struct udev_monitor * _udev_monitor;
    int _udev_monitor_fd;
    bool _changed = false;

public:
    udev_device_watcher( platform::backend const * );
    ~udev_device_watcher();

    // device_watcher
public:
    void start( platform::device_changed_callback && callback ) override;

    void stop() override;

    platform::backend_device_group get_devices() const override;

    bool is_stopped() const override { return ! _active_object.is_active(); }

private:
    void foreach_device( std::function< void( struct udev_device* udev_dev ) > );
};


}  // namespace librealsense

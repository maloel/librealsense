// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include "backend-device-group.h"

#include <functional>


namespace librealsense {
namespace platform {


typedef std::function< void( backend_device_group const & old, backend_device_group const & curr ) >
    device_changed_callback;


class device_watcher
{
public:
    virtual void start( device_changed_callback && ) = 0;
    virtual void stop() = 0;
    virtual backend_device_group get_devices() const = 0;
    virtual bool is_stopped() const = 0;
    virtual ~device_watcher() = default;
};


}  // namespace platform
}  // namespace librealsense

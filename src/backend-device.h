// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <src/device.h>
#include <memory>


namespace librealsense {


namespace platform {
class backend;
}


// Common base class for all backend devices (i.e., those that require a platform backend)
//
class backend_device : public virtual device
{
    typedef device super;

public:
    uint16_t get_pid() const { return _pid; }
    std::shared_ptr< platform::backend > get_backend();

protected:
    uint16_t _pid = 0;
};


}  // namespace librealsense

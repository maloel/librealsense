// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <memory>
#include <functional>


namespace librealsense {


class device_info;


typedef std::function< bool( std::shared_ptr< device_info > const & ) > device_info_callback;


class device_factory
{
public:
    virtual ~device_factory() = default;

    virtual void query_devices( device_info_callback ) = 0;
};


}  // namespace librealsense

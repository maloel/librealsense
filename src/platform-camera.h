// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include "backend-device.h"
#include "platform/platform-device-info.h"


namespace librealsense {


class platform_camera_info : public platform::platform_device_info
{
public:
    explicit platform_camera_info( std::shared_ptr< context > const & ctx,
                                   std::vector< platform::uvc_device_info > && uvcs )
        : platform_device_info( ctx, { std::move( uvcs ), {}, {} } )
    {
    }

    std::shared_ptr< device_interface > create_device() override;

    static std::vector< std::shared_ptr< platform_camera_info > >
    pick_uvc_devices( const std::shared_ptr< context > & ctx,
                      const std::vector< platform::uvc_device_info > & uvc_devices );
};


class platform_camera : public backend_device
{
    std::shared_ptr< const platform_camera_info > const _dev_info;

public:
    platform_camera( std::shared_ptr< const platform_camera_info > const & dev_info,
                     const std::vector< platform::uvc_device_info > & uvc_infos );

    std::shared_ptr< const device_info > get_device_info() const override { return _dev_info; }

    virtual rs2_intrinsics get_intrinsics( unsigned int, const stream_profile & ) const { return rs2_intrinsics{}; }

    std::vector< tagged_profile > get_profiles_tags() const override;
};


}  // namespace librealsense

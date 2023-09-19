// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include "backend-device-group.h"
#include <src/device-info.h>

#include <memory>
#include <set>


namespace librealsense {


class context;
class backend_device_factory;


namespace platform {


// A device_info that stores a backend_device_group
//
class platform_device_info : public device_info
{
    // This flag is maintained by the backend_device_factory, responsible for the device-callback etc.
    bool _is_alive = false;
    friend class backend_device_factory;

protected:
    platform::backend_device_group _group;

public:
    platform_device_info( std::shared_ptr< context > const & ctx, platform::backend_device_group && bdg )
        : device_info( ctx )
        , _group( std::move( bdg ) )
    {
    }

    bool is_alive() const override;

    std::string get_address() const override
    {
        for( auto & d : _group.uvc_devices )
            return d.device_path;
        for( auto & d : _group.usb_devices )
            return d.id;
        throw std::runtime_error( "non-standard platform-device-info" );
    }

    void to_stream( std::ostream & os ) const override
    {
        os << (std::string) _group;
    }

    auto & get_group() const { return _group; }

    bool is_same_as( std::shared_ptr< const device_info > const & other ) const override
    {
        if( auto rhs = std::dynamic_pointer_cast< const platform_device_info >( other ) )
            // NOTE: USB devices weren't compared by backend_device_group!
            return _group == rhs->_group;
        return false;
    }
};


inline bool operator<( std::shared_ptr< platform_device_info > const & left, std::shared_ptr< platform_device_info > const & right )
{
    auto const & left_uvc = left->get_group().uvc_devices;
    auto const & right_uvc = right->get_group().uvc_devices;
    if( left_uvc.empty() )
        return ! right_uvc.empty();
    if( right_uvc.empty() )
        return false;
    return left_uvc.front() < right_uvc.front();
}


typedef std::set< std::shared_ptr< platform_device_info > > backend_device_set;


}  // namespace platform
}  // namespace librealsense

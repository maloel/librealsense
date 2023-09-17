// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include "platform-utils.h"

#include "uvc-device-info.h"
#include <src/librealsense-exception.h>


namespace librealsense {
namespace platform {


std::vector< std::vector< uvc_device_info > >
group_devices_by_unique_id( const std::vector< uvc_device_info > & devices )
{
    std::map< std::string, std::vector< uvc_device_info > > map;
    for( auto && info : devices )
    {
        map[info.unique_id].push_back( info );
    }
    std::vector< std::vector< uvc_device_info > > result;
    for( auto && kvp : map )
    {
        result.push_back( kvp.second );
    }
    return result;
}

// TODO: Sergey
// Make template
void trim_device_list( std::vector< usb_device_info > & devices, const std::vector< usb_device_info > & chosen )
{
    if( chosen.empty() )
        return;

    auto was_chosen = [&chosen]( const usb_device_info & info )
    {
        return find( chosen.begin(), chosen.end(), info ) != chosen.end();
    };
    devices.erase( std::remove_if( devices.begin(), devices.end(), was_chosen ), devices.end() );
}

void trim_device_list( std::vector< uvc_device_info > & devices, const std::vector< uvc_device_info > & chosen )
{
    if( chosen.empty() )
        return;

    auto was_chosen = [&chosen]( const uvc_device_info & info )
    {
        return find( chosen.begin(), chosen.end(), info ) != chosen.end();
    };
    devices.erase( std::remove_if( devices.begin(), devices.end(), was_chosen ), devices.end() );
}

bool mi_present( const std::vector< uvc_device_info > & devices, uint32_t mi )
{
    for( auto && info : devices )
    {
        if( info.mi == mi )
            return true;
    }
    return false;
}

uvc_device_info get_mi( const std::vector< uvc_device_info > & devices, uint32_t mi )
{
    for( auto && info : devices )
    {
        if( info.mi == mi )
            return info;
    }
    throw invalid_value_exception( "Interface not found!" );
}

std::vector< uvc_device_info > filter_by_mi( const std::vector< uvc_device_info > & devices, uint32_t mi )
{
    std::vector< uvc_device_info > results;
    for( auto && info : devices )
    {
        if( info.mi == mi )
            results.push_back( info );
    }
    return results;
}


}  // namespace platform
}  // namespace librealsense

// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include "info.h"
#include <src/librealsense-exception.h>


namespace librealsense {


bool info_container::supports_info( rs2_camera_info info ) const
{
    auto it = _camera_info.find( info );
    return it != _camera_info.end();
}

void info_container::register_info( rs2_camera_info info, const std::string & val )
{
    if( info_container::supports_info( info ) && (info_container::get_info( info ) != val) ) // Append existing infos
    {
        _camera_info[info] += "\n" + val;
    }
    else
    {
        _camera_info[info] = val;
    }
}

void info_container::update_info( rs2_camera_info info, const std::string & val )
{
    if( info_container::supports_info( info ) )
    {
        _camera_info[info] = val;
    }
}
const std::string & info_container::get_info( rs2_camera_info info ) const
{
    auto it = _camera_info.find( info );
    if( it == _camera_info.end() )
        throw invalid_value_exception( "Selected camera info is not supported for this camera!" );

    return it->second;
}
void info_container::create_snapshot( std::shared_ptr<info_interface> & snapshot ) const
{
    snapshot = std::make_shared<info_container>( *this );
}
void info_container::enable_recording( std::function<void( const info_interface & )> record_action )
{
    //info container is a read only class, nothing to record
}

void info_container::update( std::shared_ptr<extension_snapshot> ext )
{
    if( auto && info_api = As<info_interface>( ext ) )
    {
        for( int i = 0; i < RS2_CAMERA_INFO_COUNT; ++i )
        {
            rs2_camera_info info = static_cast<rs2_camera_info>(i);
            if( info_api->supports_info( info ) )
            {
                register_info( info, info_api->get_info( info ) );
            }
        }
    }
}


}
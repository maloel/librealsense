// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp>

#include "eth-config.h"

#include <tclap/CmdLine.h>
#include <tclap/ValueArg.h>

#include <rsutils/os/special-folder.h>
//#include <rsutils/easylogging/easyloggingpp.h>
#include <rsutils/json.h>
#include <rsutils/json-config.h>
#include <rsutils/string/from.h>

#include <iostream>

using namespace TCLAP;
using rsutils::json;


static json load_settings( json const & local_settings )
{
    // Load the realsense configuration file settings
    std::string const filename = rsutils::os::get_special_folder( rsutils::os::special_folder::app_data ) + RS2_CONFIG_FILENAME;
    auto config = rsutils::json_config::load_from_file( filename );

    // Take just the 'context' part
    config = rsutils::json_config::load_settings( config, "context", "config-file" );

    // Patch the given local settings into the configuration
    config.override( local_settings, "local settings" );

    return config;
}


uint32_t const GET_ETH_CONFIG = 0xBB;
uint32_t const SET_ETH_CONFIG = 0xBA;


#define LOG_DEBUG( ... )                                                                                               \
    do                                                                                                                 \
    {                                                                                                                  \
        std::ostringstream os__;                                                                                       \
        os__ << __VA_ARGS__;                                                                                           \
        rs2_log( RS2_LOG_SEVERITY_DEBUG, os__.str().c_str(), nullptr );                                                \
    }                                                                                                                  \
    while( false )


int main( int argc, char * argv[] )
try
{
    CmdLine cmd( "librealsense rs-dds-config tool", ' ', RS2_API_FULL_VERSION_STR );
    SwitchArg debug_arg( "", "debug", "Enable debug logging", false );
    SwitchArg golden_arg( "", "golden", "Return the read-only golden values (rather than the actual)", false );
    ValueArg< std::string > sn_arg( "", "serial-number", "S/N", false, "",
                                    "Device serial-number to use, if more than one device is available" );

    cmd.add( debug_arg );
    cmd.add( golden_arg );
    cmd.add( sn_arg );
    cmd.parse( argc, argv );

    bool const golden = golden_arg.isSet();

    rs2::log_to_console( debug_arg.isSet() ? RS2_LOG_SEVERITY_DEBUG : RS2_LOG_SEVERITY_ERROR );

    // Create a RealSense context and look for a device
    json settings = load_settings( {
        { "dds", false },  // Don't discover ethernet devices; we want local devices only 
    } );
    rs2::context ctx( settings.dump() );

    auto device_list = ctx.query_devices();
    rs2::device device;
    eth_config config;
    std::string sn;
    if( sn_arg.isSet() )
        sn = sn_arg.getValue();
    auto n_devices = device_list.size();
    for( uint32_t i = 0; i < n_devices; ++i )
    {
        try
        {
            if( auto possible_device = rs2::debug_protocol( device_list[i] ) )
            {
                if( ! sn.empty() && sn != possible_device.get_info( RS2_CAMERA_INFO_SERIAL_NUMBER ) )
                    continue;
                LOG_DEBUG( "trying " << possible_device.get_description() );
                auto cmd = possible_device.build_command( GET_ETH_CONFIG, golden ? 0 : 1 );  // 0=golden; 1=actual
                LOG_DEBUG( "cmd : " << rsutils::string::hexdump( cmd.data(), cmd.size() ) );
                auto data = possible_device.send_and_receive_raw_data( cmd );
                int32_t const & code = *reinterpret_cast< int32_t const * >( data.data() );
                if( data.size() < sizeof( code ) )
                    throw std::runtime_error( rsutils::string::from()
                                              << "bad response "
                                              << rsutils::string::hexdump( data.data(), data.size() ) );
                if( code < 0 )
                    throw std::runtime_error( rsutils::string::from() << "bad response " << code );
                LOG_DEBUG( "data: " << rsutils::string::hexdump( data.data(), data.size() ).format( "{4} {repeat:}{1}{:}" ) );
                data.erase( data.begin(), data.begin() + sizeof( code ) );

                eth_config possible_config( data );
                if( device )
                {
                    std::cerr << "-F- More than one device is available; please use --serial-number <>" << std::endl;
                    return EXIT_FAILURE;
                }
                device = possible_device;
                config = possible_config;
            }
        }
        catch( std::exception const & e )
        {
            LOG_DEBUG( "failed! " << e.what() );
            continue;
        }
    }
    if( ! device )
    {
        if( sn.empty() )
            std::cerr << "-F- No device found supporting Eth" << std::endl;
        else
            std::cerr << "-F- Device not found or does not support Eth" << std::endl;
        return EXIT_FAILURE;
    }

    std::ostream & os = std::cout;

    os << "Device: " << device.get_description() << std::endl;
    os << "  MAC address: " << config.mac_address << std::endl;
    os << "  configured: " << config.configured << std::endl;
    if( config.actual && config.actual != config.configured )
        os << "  actual    : " << config.actual << std::endl;
    os << "  DDS: " << std::endl;
    os << "    domain ID: " << config.dds.domain_id << std::endl;
    os << "  link: ";
    if( ! golden )
    {
        if( config.link.speed )
            os << config.link.speed << " Mbps";
        else
            os << "OFF";
    }
    os << std::endl;
    os << "    MTU, bytes: " << config.link.mtu << std::endl;
    os << "    timeout, ms: " << config.link.timeout << std::endl;
    os << "    priority: " << config.link.priority << std::endl;
    os << "  DHCP: " << ( config.dhcp.on ? "ON" : "OFF" ) << std::endl;
    os << "    timeout, sec: " << config.dhcp.timeout << std::endl;

    return EXIT_SUCCESS;
}
catch( const rs2::error & e )
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args()
              << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch( const std::exception & e )
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}

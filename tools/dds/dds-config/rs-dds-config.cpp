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
#include <rsutils/ios/field.h>
#include <rsutils/ios/indent.h>

#include <iostream>

using namespace TCLAP;
using rsutils::json;
using rsutils::ios::field;


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


template< class T >
struct _output_field
{
    T const & _current;
    T const & _requested;
    char const * const _name;
    
    _output_field( const char * name, T const & current, T const & requested )
        : _name( name )
        , _current( current )
        , _requested( requested )
    {
    }
};
template< class T >
std::ostream & operator<<( std::ostream & os, _output_field< T > const & f )
{
    os << f._name << ':' << field::value << f._current;
    if( f._requested != f._current )
        os << field::value << "-->" << field::value << f._requested;
    return os;
}
template< class T >
_output_field< T > output_field( const char * name, T const & current )
{
    return { name, current, current };
}
template< class T >
_output_field< T > output_field( const char * name, T const & current, T const & requested )
{
    return { name, current, requested };
}


int main( int argc, char * argv[] )
try
{
    CmdLine cmd( "librealsense rs-dds-config tool", ' ', RS2_API_FULL_VERSION_STR );
    SwitchArg debug_arg( "", "debug", "Enable debug logging" );
    SwitchArg golden_arg( "", "golden", "Return the read-only golden values (rather than the actual)" );
    ValueArg< std::string > sn_arg( "", "serial-number",
                                    "Device serial-number to use, if more than one device is available",
                                    false, "", "S/N" );
    ValueArg< std::string > ip_arg( "", "ip",
                                    "Device static IP address to use when DHCP is off",
                                    false, "", "1.2.3.4" );
    ValueArg< std::string > mask_arg( "", "mask",
                                    "Device static IP network mask to use when DHCP is off",
                                    false, "", "1.2.3.4" );
    ValueArg< std::string > gateway_arg( "", "gateway",
                                      "Device static IP network mask to use when DHCP is off",
                                      false, "", "1.2.3.4" );
    SwitchArg usb_only_arg( "", "usb-only", "Configure device to always use USB; never Ethernet" );
    SwitchArg usb_first_arg( "", "usb-first", "Configure device to prioritize USB before Ethernet" );
    SwitchArg eth_only_arg( "", "eth-only", "Configure device to always use Ethernet; never USB" );
    SwitchArg eth_first_arg( "", "eth-first", "Configure device to prioritize Ethernet over USB (the default)" );

    cmd.add( debug_arg );
    cmd.add( golden_arg );
    cmd.add( sn_arg );
    cmd.add( ip_arg );
    cmd.add( mask_arg );
    cmd.add( gateway_arg );
    cmd.add( usb_only_arg );
    cmd.add( usb_first_arg );
    cmd.add( eth_only_arg );
    cmd.add( eth_first_arg );
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
    eth_config current;
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
                if( code != GET_ETH_CONFIG )
                    throw std::runtime_error( rsutils::string::from() << "bad response " << code );
                LOG_DEBUG( "data: " << rsutils::string::hexdump( data.data(), data.size() ).format( "{4} {repeat:}{1}{:}" ) );
                data.erase( data.begin(), data.begin() + sizeof( code ) );

                eth_config config( data );
                if( device )
                {
                    std::cerr << "-F- More than one device is available; please use --serial-number <>" << std::endl;
                    return EXIT_FAILURE;
                }
                device = possible_device;
                current = config;
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

    eth_config requested( current );
    if( golden )
    {
        if( ip_arg.isSet() || mask_arg.isSet() || usb_only_arg.isSet() || usb_first_arg.isSet() || eth_only_arg.isSet()
            || eth_first_arg.isSet() )
        {
            throw std::runtime_error( "Cannot change any settings with --golden" );
        }
    }
    else
    {
        if( ip_arg.isSet() )
            requested.configured.ip = rsutils::string::ip_address( ip_arg.getValue(), rsutils::throw_if_not_valid );
        if( mask_arg.isSet() )
            requested.configured.netmask = rsutils::string::ip_address( ip_arg.getValue(), rsutils::throw_if_not_valid );
        if( gateway_arg.isSet() )
            requested.configured.gateway = rsutils::string::ip_address( ip_arg.getValue(), rsutils::throw_if_not_valid );
        if( usb_only_arg.isSet() + usb_first_arg.isSet() + eth_only_arg.isSet() + eth_first_arg.isSet() > 1 )
            throw std::runtime_error( "--usb-only, --usb-first, --eth-only, and --eth-first are mutually exclusive" );
        if( usb_only_arg.isSet() )
            requested.link.priority = link_priority::usb_only;
        else if( usb_first_arg.isSet() )
            requested.link.priority = link_priority::usb_first;
        else if( eth_only_arg.isSet() )
            requested.link.priority = link_priority::eth_only;
        else if( eth_first_arg.isSet() )
            requested.link.priority = link_priority::eth_first;
    }

    std::ostream & os = std::cout;
    {
        field::group device_group;
        os << "Device: " << device.get_description() << rsutils::ios::indent() << device_group;
        {
            os << field::separator << output_field( "MAC address", current.mac_address );
            os << field::separator << output_field( "configured", current.configured, requested.configured );
            if( current.actual && current.actual != current.configured )
                os << field::separator << output_field( "actual    ", current.actual );

            {
                field::group dds_group;
                os << field::separator << "DDS:" << dds_group;
                os << field::separator << output_field( "domain ID", current.dds.domain_id );
            }
            {
                os << field::separator << "link:" << field::value;
                if( ! golden )
                {
                    if( current.link.speed )
                        os << current.link.speed << " Mbps";
                    else
                        os << "OFF";
                }
                field::group link_group;
                os << link_group;
                os << field::separator << output_field( "MTU, bytes", current.link.mtu );
                os << field::separator << output_field( "timeout, ms", current.link.timeout );
                os << field::separator << output_field( "priority", current.link.priority, requested.link.priority );
            }
            {
                std::string current_dhcp( current.dhcp.on ? "ON" : "OFF" );
                std::string requested_dhcp( requested.dhcp.on ? "ON" : "OFF" );
                os << field::separator << output_field( "DHCP", current_dhcp, requested_dhcp );
                field::group dhcp_group;
                os << dhcp_group;
                os << field::separator << output_field( "timeout, sec", current.dhcp.timeout );
            }
        }
    }
    os << std::endl;

    return EXIT_SUCCESS;
}
catch( const rs2::error & e )
{
    std::cerr << "-F- RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args()
              << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch( const std::exception & e )
{
    std::cerr << "-F- " << e.what() << std::endl;
    return EXIT_FAILURE;
}

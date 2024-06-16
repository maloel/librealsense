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
char const * const HWM_FMT = "{4} {04}({4i},{4i},{4i},{4i}) {repeat:}{1}{:}";


#define LOG_DEBUG( ... )                                                                                               \
    do                                                                                                                 \
    {                                                                                                                  \
        std::ostringstream os__;                                                                                       \
        os__ << __VA_ARGS__;                                                                                           \
        rs2_log( RS2_LOG_SEVERITY_DEBUG, os__.str().c_str(), nullptr );                                                \
    }                                                                                                                  \
    while( false )
bool g_quiet = false;
#define INFO( ... )                                                                                                    \
    do                                                                                                                 \
    {                                                                                                                  \
        if( ! g_quiet )                                                                                                \
            std::cout << "-I- " << __VA_ARGS__ << std::endl;                                                           \
    }                                                                                                                  \
    while( false )


struct indent
{
    int spaces;
    indent( int spaces ) : spaces( spaces ) {}
};
std::ostream & operator<<( std::ostream & os, indent const & ind )
{
    for( int i = ind.spaces; i; --i )
        os << ' ';
    return os;
}


template< class T >
struct _output_field
{
    char const * const _name;
    T const & _current;
    T const & _requested;
};
template< class T >
std::ostream & operator<<( std::ostream & os, _output_field< T > const & f )
{
    os << f._name << ": " << f._current;
    if( f._requested != f._current )
        os << " --> " << f._requested;
    return os;
}
template< class T >
_output_field< T > setting( const char * name, T const & current )
{
    return { name, current, current };
}
template< class T >
_output_field< T > setting( const char * name, T const & current, T const & requested )
{
    return { name, current, requested };
}


eth_config get_eth_config( rs2::debug_protocol hwm, bool golden )
{
    auto cmd = hwm.build_command( GET_ETH_CONFIG, golden ? 0 : 1 );  // 0=golden; 1=actual
    LOG_DEBUG( "cmd: " << rsutils::string::hexdump( cmd.data(), cmd.size() ).format( HWM_FMT ) );
    auto data = hwm.send_and_receive_raw_data( cmd );
    int32_t const & code = *reinterpret_cast<int32_t const *>(data.data());
    if( data.size() < sizeof( code ) )
        throw std::runtime_error( rsutils::string::from()
                                  << "bad response " << rsutils::string::hexdump( data.data(), data.size() ) );
    if( code != GET_ETH_CONFIG )
        throw std::runtime_error( rsutils::string::from() << "bad response " << code );
    LOG_DEBUG( "response: " << rsutils::string::hexdump( data.data(), data.size() ).format( "{4} {repeat:}{1}{:}" ) );
    data.erase( data.begin(), data.begin() + sizeof( code ) );

    return eth_config( data );
}


int main( int argc, char * argv[] )
try
{
    CmdLine cmd( "librealsense rs-dds-config tool", ' ', RS2_API_FULL_VERSION_STR );
    SwitchArg debug_arg( "", "debug", "Enable debug (-D-) output; by default only errors (-E-) are shown" );
    SwitchArg quiet_arg( "", "quiet", "Suppress regular informational (-I-) messages" );
    SwitchArg no_reset_arg( "", "no-reset", "Do not hardware reset after changes are made" );
    SwitchArg golden_arg( "", "golden", "Show R/O golden values vs. current; mutually exclusive with any changes" );
    SwitchArg factory_reset_arg( "", "factory-reset", "Reset settings back to the --golden values" );
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
    ValueArg< std::string > dhcp_arg( "", "dhcp",
                                      "DHCP dynamic IP discovery 'on' or 'off'",
                                      false, "on", "on/off" );
    ValueArg< uint32_t > dhcp_timeout_arg( "", "dhcp-timout",
                                           "Seconds before DHCP times out and falls back to a static IP",
                                           false, 30, "seconds" );
    ValueArg< uint32_t > link_timeout_arg( "", "link-timout",
                                           "Milliseconds before --eth-first link times out and falls back to USB",
                                           false, 4000, "milliseconds" );
    ValueArg< uint8_t > domain_id_arg( "", "domain-id",
                                       "DDS Domain ID to use (default is 0)",
                                       false, 0, "0-232" );
    SwitchArg usb_only_arg( "", "usb-only", "Configure device to always use USB; never Ethernet" );
    SwitchArg usb_first_arg( "", "usb-first", "Configure device to prioritize USB before Ethernet" );
    SwitchArg eth_only_arg( "", "eth-only", "Configure device to always use Ethernet; never USB" );
    SwitchArg eth_first_arg( "", "eth-first", "Configure device to prioritize Ethernet over USB (the default)" );

    // In reverse order to how they will be listed
    cmd.add( no_reset_arg );
    cmd.add( domain_id_arg );
    cmd.add( gateway_arg );
    cmd.add( mask_arg );
    cmd.add( ip_arg );
    cmd.add( dhcp_timeout_arg );
    cmd.add( dhcp_arg );
    cmd.add( link_timeout_arg );
    cmd.add( eth_first_arg );
    cmd.add( eth_only_arg );
    cmd.add( usb_first_arg );
    cmd.add( usb_only_arg );
    cmd.add( factory_reset_arg );
    cmd.add( golden_arg );
    cmd.add( sn_arg );
    cmd.add( quiet_arg );
    cmd.add( debug_arg );
    cmd.parse( argc, argv );

    g_quiet = quiet_arg.isSet();
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
                current = get_eth_config( possible_device, golden );
                if( device )
                    throw std::runtime_error( "More than one device is available; please use --serial-number <>" );
                device = possible_device;
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
            throw std::runtime_error( "No device found supporting Eth" );
        else
            throw std::runtime_error( "Device not found or does not support Eth" );
    }
    INFO( "Device: " << device.get_description() );

    eth_config requested( current );
    if( golden || factory_reset_arg.isSet() )
    {
        if( ip_arg.isSet() || mask_arg.isSet() || usb_only_arg.isSet() || usb_first_arg.isSet() || eth_only_arg.isSet()
            || eth_first_arg.isSet() || link_timeout_arg.isSet() || dhcp_arg.isSet() || dhcp_timeout_arg.isSet()
            || golden == factory_reset_arg.isSet() )
        {
            throw std::runtime_error( "Cannot change any settings with --golden" );
        }
        if( factory_reset_arg.isSet() )
        {
            LOG_DEBUG( "getting golden:" );
            requested = get_eth_config( device, true );  // golden
        }
        else
        {
            // Current is the golden values; show them vs the actual values in requested
            // No actual changes will be made
            LOG_DEBUG( "getting current:" );
            requested = get_eth_config( device, false );  // not golden
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
            throw std::invalid_argument( "--usb-only, --usb-first, --eth-only, and --eth-first are mutually exclusive" );
        if( usb_only_arg.isSet() )
            requested.link.priority = link_priority::usb_only;
        else if( usb_first_arg.isSet() )
            requested.link.priority = link_priority::usb_first;
        else if( eth_only_arg.isSet() )
            requested.link.priority = link_priority::eth_only;
        else if( eth_first_arg.isSet() )
            requested.link.priority = link_priority::eth_first;
        if( link_timeout_arg.isSet() )
            requested.link.timeout = link_timeout_arg.getValue();
        if( dhcp_arg.isSet() )
        {
            if( dhcp_arg.getValue() == "on" )
                requested.dhcp.on = true;
            else if( dhcp_arg.getValue() == "off" )
                requested.dhcp.on = false;
            else
                throw std::invalid_argument( "--dhcp can be 'on' or 'off'; got " + dhcp_arg.getValue() );
        }
        if( dhcp_timeout_arg.isSet() )
            requested.dhcp.timeout = dhcp_timeout_arg.getValue();
        if( domain_id_arg.isSet() )
        {
            if( domain_id_arg.getValue() > 232 )
                throw std::invalid_argument( "--domain-id must be 0-232" );
            requested.dds.domain_id = domain_id_arg.getValue();
        }
    }

    if( ! g_quiet )
    {
        INFO( indent( 4 ) << setting( "MAC address", current.mac_address ) );
        INFO( indent( 4 ) << setting( "configured", current.configured, requested.configured ) );
        if( current.actual && current.actual != current.configured )
            INFO( indent( 4 ) << setting( "actual    ", current.actual ) );

        {
            INFO( indent( 4 ) << "DDS:" );
            INFO( indent( 8 ) << setting( "domain ID", current.dds.domain_id, requested.dds.domain_id ) );
        }
        {
            if( golden )
            {
                INFO( indent( 4 ) << "link:" );
            }
            else
            {
                if( current.link.speed )
                    INFO( indent( 4 ) << setting( "link", current.link.speed ) << " Mbps" );
                else
                    INFO( indent( 4 ) << setting( "link", "OFF" ) );
            }
            INFO( indent( 8 ) << setting( "MTU, bytes", current.link.mtu ) );
            INFO( indent( 8 ) << setting( "timeout, ms", current.link.timeout, requested.link.timeout ) );
            INFO( indent( 8 ) << setting( "priority", current.link.priority, requested.link.priority ) );
        }
        {
            std::string current_dhcp( current.dhcp.on ? "ON" : "OFF" );
            std::string requested_dhcp( requested.dhcp.on ? "ON" : "OFF" );
            INFO( indent( 4 ) << setting( "DHCP", current_dhcp, requested_dhcp ) );
            INFO( indent( 8 ) << setting( "timeout, sec", current.dhcp.timeout, requested.dhcp.timeout ) );
        }
    }

    if( golden )
    {
        LOG_DEBUG( "no changes requested with --golden" );
    }
    else if( requested == current )
    {
        LOG_DEBUG( "nothing to do" );
    }
    else
    {
        rs2::debug_protocol hwm( device );
        auto cmd = hwm.build_command( SET_ETH_CONFIG, 0, 0, 0, 0, requested.build_command() );
        LOG_DEBUG( "cmd: " << rsutils::string::hexdump( cmd.data(), cmd.size() ).format( HWM_FMT ) );
        auto data = hwm.send_and_receive_raw_data( cmd );
        int32_t const & code = *reinterpret_cast< int32_t const * >( data.data() );
        if( data.size() != sizeof( code ) )
            throw std::runtime_error( rsutils::string::from()
                                      << "Failed to change: bad response size " << data.size() << ' '
                                      << rsutils::string::hexdump( data.data(), data.size() ) );
        if( code != SET_ETH_CONFIG )
            throw std::runtime_error( rsutils::string::from() << "Failed to change: bad response " << code );
        INFO( "Successfully changed" );
        if( ! no_reset_arg.isSet() )
        {
            INFO( "Resetting device..." );
            device.hardware_reset();
        }
    }

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

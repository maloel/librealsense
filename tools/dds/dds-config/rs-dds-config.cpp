// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp>

#include <tclap/CmdLine.h>
#include <tclap/ValueArg.h>

#include <rsutils/os/special-folder.h>
//#include <rsutils/easylogging/easyloggingpp.h>
#include <rsutils/json.h>
#include <rsutils/json-config.h>
#include <rsutils/string/from.h>
#include <rsutils/string/hexdump.h>
#include <rsutils/string/ip-address.h>
#include <rsutils/number/crc32.h>

#include <iostream>

using namespace TCLAP;
using rsutils::json;
using rsutils::string::ip_address;


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


// The header structure for eth config
#pragma pack( push, 1 )
typedef struct {
    uint16_t version;
    uint16_t size;  // without header
    uint32_t crc;   // without header
} eth_config_header;

// The structure data for eth config info
typedef struct {
    eth_config_header header;
    uint32_t link_check_timeout;  // The threshold to wait eth link(ms).
    uint8_t config_ip[4];  // static IP
    uint8_t config_netmask[4];  // static netmask
    uint8_t config_gateway[4];  // static gateway
    uint8_t actual_ip[4];  // actual IP when dhcp is ON, read-only
    uint8_t actual_netmask[4];  // actual netmask when dhcp is ON, read-only
    uint8_t actual_gateway[4];  // actual gateway when dhcp is ON, read-only
    uint32_t link_speed;  // Mbps read-only
    uint32_t mtu;  // read-only
    uint8_t dhcp_on;
    uint8_t dhcp_timeout;  // The threshold to wait valid ip when DHCP is on(s).
    uint8_t domain_id;  // dds domain id
    uint8_t link_priority;  // device link priority. 0-USB_ONLY, 1-ETH_ONLY, 2-ETH_FIRST, 3 USB_FIRST
    uint8_t mac_address[6];  // read-only
    uint8_t reserved[2];
} eth_config_v3;
#pragma pack( pop )


enum class link_priority
{
    usb_only = 0,
    eth_only = 1,
    eth_first = 2,
    usb_first = 3
};


std::ostream & operator<<( std::ostream & os, link_priority p )
{
    switch( p )
    {
    case link_priority::usb_only: os << "usb-only"; break;
    case link_priority::usb_first: os << "usb-first"; break;
    case link_priority::eth_only: os << "eth-only"; break;
    case link_priority::eth_first: os << "eth-first"; break;
    default:
        os << "UNKNOWN-" << (int) p;
        break;
    }
    return os;
}


struct ip_setting
{
    ip_address ip;
    ip_address netmask;
    ip_address gateway;
};


std::ostream & operator<<( std::ostream & os, ip_setting const & setting )
{
    os << setting.ip;
    if( setting.netmask.is_valid() )
        os << " mask " << setting.netmask;
    if( setting.gateway.is_valid() )
        os << " gateway " << setting.gateway;
    return os;
}


struct eth_config {
    eth_config_header header;
    std::string mac_address;
    ip_setting configured;
    ip_setting actual;
    struct {
        int domain_id;  // dds domain id
    } dds;
    struct
    {
        unsigned mtu;      // bytes
        unsigned speed;    // Mbps read-only
        unsigned timeout;  // The threshold to wait eth link(ms)
        link_priority priority;
    } link;
    struct
    {
        bool on;
        int timeout;  // The threshold to wait valid ip when DHCP is on(s)
    } dhcp;

    eth_config()
    {}

    eth_config( eth_config_v3 const & v3 )
        : header( v3.header )
        , mac_address( rsutils::string::from( rsutils::string::hexdump( v3.mac_address, sizeof( v3.mac_address ) )
                                                  .format( "{01}:{01}:{01}:{01}:{01}:{01}" ) ) )
        , configured{ v3.config_ip, v3.config_netmask, v3.config_gateway }
        , actual{ v3.actual_ip, v3.actual_netmask, v3.actual_gateway }
        , dds{ v3.domain_id }
        , link{ v3.mtu, v3.link_speed, v3.link_check_timeout, link_priority( v3.link_priority ) }
        , dhcp{ v3.dhcp_on != 0, v3.dhcp_timeout }
    {}
};


#define LOG_DEBUG( ... )                                                                                               \
    do                                                                                                                 \
    {                                                                                                                  \
        std::ostringstream os__;                                                                                       \
        os__ << __VA_ARGS__;                                                                                           \
        rs2_log( RS2_LOG_SEVERITY_DEBUG, os__.str().c_str(), nullptr );                                                \
    }                                                                                                                  \
    while( false )


void dump_data( std::vector< uint8_t > const & data )
{
    LOG_DEBUG( "data: " << rsutils::string::hexdump( data.data(), data.size() ) );
}


eth_config verify_eth_config( std::vector< uint8_t > const & data )
{
    dump_data( data );
    int32_t const & code = *reinterpret_cast< int32_t const * >( data.data() );
    if( code < 0 )
        throw std::runtime_error( rsutils::string::from() << "bad response " << code );
    if( data.size() != sizeof( eth_config_v3 ) + sizeof( code ) )
        throw std::runtime_error( rsutils::string::from()
                                  << "Eth config table v3 size (" << sizeof( code ) << "+" << sizeof( eth_config_v3 )
                                  << ") does not match response size (" << data.size() << ")" );
    auto config = reinterpret_cast< eth_config_v3 const * >( data.data() + sizeof( code ) );
    if( config->header.version != 3 )
        throw std::runtime_error( rsutils::string::from() << "invalid Eth config table version " << config->header.version );
    if( config->header.size != sizeof( eth_config_v3 ) - sizeof( eth_config_header ) )
        throw std::runtime_error( rsutils::string::from()
                                  << "invalid Eth config table v3 size (" << config->header.size << "); expecting "
                                  << sizeof( eth_config_v3 ) << "-" << sizeof( eth_config_header ) );
    auto const crc = rsutils::number::calc_crc32( data.data() + sizeof( code ) + sizeof( eth_config_header ),
                                                  config->header.size );
    if( config->header.crc != crc )
        throw std::runtime_error( rsutils::string::from() << "Eth config table v3 crc (" << config->header.crc
                                                          << ") does not match calculated " << crc );
    return *config;
}


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
                auto cmd = possible_device.build_command( GET_ETH_CONFIG, golden_arg.isSet() ? 0 : 1 );  // 0=golden; 1=actual
                LOG_DEBUG( "cmd : " << rsutils::string::hexdump( cmd.data(), cmd.size() ) );
                auto data = possible_device.send_and_receive_raw_data( cmd );
                auto possible_config = verify_eth_config( data );
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

    std::cout << "Device: " << device.get_description() << std::endl;
    std::cout << "  MAC address: " << config.mac_address << std::endl;
    std::cout << "  configured: " << config.configured << std::endl;
    std::cout << "  actual    : " << config.actual << std::endl;
    std::cout << "  DDS: " << std::endl;
    std::cout << "    domain ID: " << config.dds.domain_id << std::endl;
    std::cout << "  link: " << std::endl;
    std::cout << "    MTU, bytes: " << config.link.mtu << std::endl;
    std::cout << "    speed, Mbps: " << config.link.speed << std::endl;
    std::cout << "    timeout, ms: " << config.link.timeout << std::endl;
    std::cout << "    priority: " << config.link.priority << std::endl;
    std::cout << "  DHCP: " << ( config.dhcp.on ? "ON" : "OFF" ) << std::endl;
    std::cout << "    timeout, sec: " << config.dhcp.timeout << std::endl;

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

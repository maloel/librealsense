// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include <rscore/context.h>
#include <rscore/device-info.h>

#include <realdds/dds-log-consumer.h>

#include <rsutils/easylogging/easyloggingpp.h>

#include <tclap/CmdLine.h>
using namespace TCLAP;

#include <nlohmann/json.hpp>

#include <iostream>
#include <iomanip>
#include <map>
#include <set>
#include <cstring>
#include <cmath>
#include <limits>
#include <thread>


int main( int argc, char ** argv )
{
    CmdLine cmd( "rsdds-devices" );

    SwitchArg debug_arg( "", "debug", "Turn on LibRS debug logs" );
    ValueArg< int > domain_arg( "", "dds-domain", "Set the DDS domain ID (default to 0)", false, 0, "0-232" );
    SwitchArg verbose_arg( "v", "verbose", "Show extra information" );

    cmd.add( debug_arg );
    cmd.add( verbose_arg );
    cmd.add( domain_arg );

    cmd.parse( argc, argv );


    // Configure the same logger as librealsense, and default to only errors by default...
    rsutils::configure_elpp_logger( debug_arg.isSet() );
    // And set the DDS logger similarly
    eprosima::fastdds::dds::Log::ClearConsumers();
    eprosima::fastdds::dds::Log::RegisterConsumer( realdds::log_consumer::create() );
    eprosima::fastdds::dds::Log::SetVerbosity( eprosima::fastdds::dds::Log::Error );

    bool verbose = verbose_arg.getValue();

    // Obtain a list of devices currently present on the system
    nlohmann::json settings;
    nlohmann::json dds;
    dds["domain"] = domain_arg.getValue();
    settings["dds"] = std::move( dds );
    librealsense::context ctx( settings );

    int mask = 0x1fe; // RS2_PRODUCT_LINE_ANY_INTEL | RS2_PRODUCT_LINE_SW_ONLY
    auto devices = ctx.query_devices( mask );
    // For SW-only devices, allow some time for DDS devices to connect
    int tries = 3;
    std::cout << "No device detected. Waiting..." << std::flush;
    while( devices.empty() && tries-- )
    {
        std::cout << "." << std::flush;
        std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
        devices = ctx.query_devices( mask );
    }
    std::cout << std::endl;

    if( devices.empty() )
    {
        std::cout << "No device detected!" << std::endl;
        return EXIT_SUCCESS;
    }

    //std::cout << std::left << std::setw( 30 ) << "Device Name" << setw( 20 ) << "Serial Number" << setw( 20 )
    //          << "Firmware Version" << std::endl;

    for( auto dev_info : devices )
    {
        auto dev = dev_info;

        std::cout << dev->get_address() << std::endl;

        //cout << left << setw( 30 ) << dev_info( dev, RS2_CAMERA_INFO_NAME ) << setw( 20 )
        //        << dev_info( dev, RS2_CAMERA_INFO_SERIAL_NUMBER ) << setw( 20 )
        //        << dev_info( dev, RS2_CAMERA_INFO_FIRMWARE_VERSION ) << endl;
    }

    return EXIT_SUCCESS;
}

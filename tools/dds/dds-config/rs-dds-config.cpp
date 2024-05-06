// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp>

#include <tclap/CmdLine.h>
#include <tclap/ValueArg.h>

#include <rsutils/os/executable-name.h>
#include <rsutils/os/special-folder.h>
#include <rsutils/easylogging/easyloggingpp.h>
#include <rsutils/json.h>
#include <rsutils/json-config.h>

#include <string>
#include <iostream>
#include <map>
#include <set>

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


int main( int argc, char * argv[] )
try
{
    CmdLine cmd( "librealsense rs-dds-config tool", ' ', RS2_API_FULL_VERSION_STR );
    SwitchArg debug_arg( "", "debug", "Enable debug logging", false );

    cmd.add( debug_arg );
    cmd.parse( argc, argv );

    rs2::log_to_console( debug_arg.isSet() ? RS2_LOG_SEVERITY_DEBUG : RS2_LOG_SEVERITY_ERROR );

    std::cout << "Starting RS DDS Adapter.." << std::endl;

    std::cout << "Start listening to RS devices.." << std::endl;

    // Create a RealSense context and look for a device
    json settings = load_settings( {
        { "dds", false },  // Don't discover ethernet devices; we want local devices only 
    } );
    rs2::context ctx( settings.dump() );

    ctx.query_devices();

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

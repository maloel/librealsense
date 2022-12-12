// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/dds-participant.h>
#include <realdds/dds-utilities.h>
#include <realdds/dds-time.h>
#include <realdds/dds-log-consumer.h>

#include <payload/op-reader.h>
#include <payload/op-writer.h>
#include <payload/streamer.h>

#include <payload/stream-payload.h>
#include <realdds/dds-topic.h>
#include <realdds/dds-topic-writer.h>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/topic/Topic.hpp>

#include <librealsense2/utilities/easylogging/easyloggingpp.h>

#include <tclap/CmdLine.h>
#include <tclap/ValueArg.h>
#include <tclap/SwitchArg.h>






int main( int argc, char ** argv ) try
{
    TCLAP::CmdLine cmd( "POC embedded server", ' ' );
    TCLAP::SwitchArg debug_arg( "", "debug", "Enable debug logging", false );
    TCLAP::ValueArg< realdds::dds_domain_id > domain_arg( "d", "domain", "select domain ID to listen on", false, 0, "0-232" );
    cmd.add( domain_arg );
    cmd.add( debug_arg );
    cmd.parse( argc, argv );

    // Configure the same logger as librealsense, and default to only errors by default...
    el::Configurations defaultConf;
    defaultConf.setToDefault();
    defaultConf.setGlobally( el::ConfigurationType::ToStandardOutput, "false" );
    defaultConf.set( el::Level::Error, el::ConfigurationType::ToStandardOutput, "true" );
    defaultConf.set( el::Level::Warning, el::ConfigurationType::ToStandardOutput, debug_arg.isSet() ? "true" : "false" );
    defaultConf.set( el::Level::Info, el::ConfigurationType::ToStandardOutput, "true" );
    defaultConf.set( el::Level::Debug, el::ConfigurationType::ToStandardOutput, debug_arg.isSet() ? "true" : "false" );
    defaultConf.setGlobally( el::ConfigurationType::Format, "-%levshort- %datetime{%H:%m:%s.%g} %msg (%fbase:%line [%thread])" );
    el::Loggers::reconfigureLogger( "librealsense", defaultConf );
    
    // And set the DDS logger similarly
    eprosima::fastdds::dds::Log::ClearConsumers();
    eprosima::fastdds::dds::Log::RegisterConsumer( realdds::log_consumer::create() );
    eprosima::fastdds::dds::Log::SetVerbosity( eprosima::fastdds::dds::Log::Error );

    realdds::dds_domain_id domain = 0;
    if( domain_arg.isSet() )
    {
        domain = domain_arg.getValue();
        if( domain > 232 )
        {
            LOG_ERROR( "Invalid domain value, enter a value in the range [0, 232]" );
            return EXIT_FAILURE;
        }
    }


    auto participant = std::make_shared< realdds::dds_participant >();
    participant->init( domain, "poc-e" );

    poc::op_writer e2h( participant, "realsense/e2h" );
    poc::op_reader h2e( participant, "realsense/h2e" );

    enum
    {
        RGB = 0x1,
        DEPTH = 0x2,
        GYRO = 0x4,
        IMU = 0x8,
        SAFETY = 0x10
    };
    poc::streamer depth( participant, "realsense/depth", DEPTH );
    poc::streamer rgb( participant, "realsense/rgb", RGB );
    poc::streamer gyro( participant, "realsense/gyro", GYRO );
    poc::streamer imu( participant, "realsense/imu", IMU );
    poc::streamer safety( participant, "realsense/safety", SAFETY );

    std::map< poc::op_payload::op_t, std::function< void( poc::op_payload const &, eprosima::fastdds::dds::SampleInfo const & ) > >
        ops;
    ops.emplace( poc::op_payload::EXIT, []( poc::op_payload const &, eprosima::fastdds::dds::SampleInfo const & ) {
        LOG_INFO( "Exit requested" );
        exit( 0 );
    } );
    ops.emplace( poc::op_payload::SYNC,
                 [&]( poc::op_payload const & payload, eprosima::fastdds::dds::SampleInfo const & sample ) {
                     LOG_INFO( "Ping" );
                     e2h.wait_for_readers( 1 );  // sometimes we don't see the reader until after we write...
                     e2h.write( poc::op_payload::SYNC,
                                payload._id,
                                payload._data[0],                    // t0_ before H app send
                                sample.source_timestamp.to_ns(),     // t0 "originate" H DDS send time
                                sample.reception_timestamp.to_ns(),  // t1 "receive" E receive time
                                realdds::now().to_ns() );            // t2_ E app send time
                 } );

    while( 1 )
    {
        auto data = h2e.read();
        auto it = ops.find( static_cast< poc::op_payload::op_t >( data.msg._op ) );
        if( it == ops.end() )
        {
            LOG_ERROR( "invalid op '" << data.msg._op << "' in message; ignoring" );
            enum { UNKNOWN_OP };
            e2h.write( poc::op_payload::ERROR, data.msg._id, UNKNOWN_OP );
            continue;
        }
        it->second( data.msg, data.sample );
    }

    return EXIT_SUCCESS;
}
catch( const TCLAP::ExitException & )
{
    LOG_ERROR( "Undefined exception while parsing command line arguments" );
    return EXIT_FAILURE;
}
catch( const TCLAP::ArgException & e )
{
    LOG_ERROR( e.what() );
    return EXIT_FAILURE;
}
catch( const std::exception & e )
{
    LOG_ERROR( e.what() );
    return EXIT_FAILURE;
}



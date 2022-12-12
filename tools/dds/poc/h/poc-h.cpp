// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/dds-participant.h>
#include <realdds/dds-utilities.h>
#include <realdds/dds-time.h>
#include <realdds/dds-log-consumer.h>

#include <payload/op-reader.h>
#include <payload/op-writer.h>

#include <payload/stream-reader.h>

#include <librealsense2/utilities/easylogging/easyloggingpp.h>

#include <tclap/CmdLine.h>
#include <tclap/ValueArg.h>
#include <tclap/SwitchArg.h>
#include <tclap/UnlabeledValueArg.h>

#include "running-average.h"


using realdds::dds_nsec;
using realdds::dds_time;
using realdds::timestr;


// Return (a+b)/2 without any chance of overflow
//
dds_nsec calc_avg( dds_nsec a, dds_nsec b, dds_nsec * p_leftover = nullptr )
{
    dds_nsec avg = a / 2;
    avg += b / 2;
    dds_nsec leftover = (a % 2 + b % 2);
    avg += leftover / 2;
    if( p_leftover )
        *p_leftover += leftover % 2;
    return avg;
}


dds_nsec
calc_time_offset( poc::op_writer & h2e, poc::op_reader & e2h, int const n_reps = 5 )
{
    rsutils::number::running_average< dds_nsec > running_offset_avg;
    for( uint64_t i = 0; i < n_reps; ++i )
    {
        auto t0_ = realdds::now().to_ns();
        h2e.write( poc::op_payload::SYNC, i, t0_ );
        auto data = e2h.read( std::chrono::seconds( 3 ) );
        //dds_nsec t0_ = data.msg._data[0];  // before H app send
        dds_nsec t0 = data.msg._data[1];   // "originate" H DDS send time
        dds_nsec t1 = data.msg._data[2];   // "receive" E receive time
        dds_nsec t2 = data.msg._data[3];   // E app send time
        //dds_nsec t2_ = data.sample.source_timestamp.to_ns();  // "transmit" E DDS send time
        dds_nsec t3 = data.sample.reception_timestamp.to_ns();
        dds_nsec t3_ = realdds::now().to_ns();

#define RJ(N,S) std::setw(N) << std::right << (S)

        LOG_DEBUG( "\n"
            "    E: " << RJ(45, timestr(t1,timestr::abs,timestr::no_suffix)) << " " << timestr(t2,t1) << "\n"
            "       " << RJ(44, "(" + timestr(t1+running_offset_avg.get(),t0) + ")/" )  << "   \\\n"
            "       " << RJ(43, "/")                          << "     \\\n"
            "       " << RJ(42, "/")                         << "       \\(" << timestr(t3,t2+running_offset_avg.get()) << ")\n"
            "    H: " << RJ(25, timestr(t0_,timestr::no_suffix)) << RJ(16, timestr(t0,t0_) )
                                                           << "         " << timestr(t3,t0) << "   " << RJ(13, timestr(t3_,t3) ) << "\n"
            );

        auto time_offset = calc_avg( t0, t3 ) - calc_avg( t1, t2 );
        LOG_DEBUG( "   time-offset= " << timestr( time_offset, timestr::rel ) << "    round-trip= " << timestr( t3_, t0_ ) );
        if( i > 0 ) {
            running_offset_avg.add( time_offset );
        }
    }
    //
    LOG_INFO( "Average time-offset= " << timestr( running_offset_avg.get(), timestr::rel ) );
    return running_offset_avg.get();
}


int main( int argc, char** argv ) try
{
    TCLAP::CmdLine cmd( "POC host computer", ' ' );
    TCLAP::SwitchArg debug_arg( "", "debug", "Enable debug logging", false );
    TCLAP::ValueArg< realdds::dds_domain_id > domain_arg( "d", "domain", "select domain ID to listen on", false, 0, "0-232" );
    TCLAP::UnlabeledValueArg< std::string > command_arg( "command", "command to send", false, "", "string" );
    cmd.add( domain_arg );
    cmd.add( debug_arg );
    cmd.add( command_arg );
    cmd.parse( argc, argv );

    // Configure the same logger as librealsense, and default to only errors by default...
    el::Configurations defaultConf;
    defaultConf.setToDefault();
    defaultConf.setGlobally( el::ConfigurationType::ToStandardOutput, "false" );
    defaultConf.set( el::Level::Fatal, el::ConfigurationType::ToStandardOutput, "true" );
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
    participant->init( domain, "poc-h" );

    poc::op_writer h2e( participant, "h2e" );
    h2e.wait_for_readers( 1 );
    if( command_arg.isSet() )
    {
        if( command_arg.getValue() == "exit" )
        {
            h2e.write( poc::op_payload::EXIT, 0 );
            h2e.wait_for_readers( 0 );
            exit( 0 );
        }
        LOG_ERROR( "Invalid command: " << command_arg.getValue() );
        exit( 1 );
    }

    poc::op_reader e2h( participant, "e2h" );

    auto time_offset = calc_time_offset( h2e, e2h );

#if 0
    // Tell E to start
    h2e.write( { { "op", "start" }, { "id", 0 } } );
    auto msg = e2h.read().msg.json_data();  // wait for confirmation
    auto status = utilities::json::get< int64_t >( msg, "status" );
    if( status != 0 )
        LOG_FATAL( "Got bad status " << status << " from E in response to 'start' op" );
#endif

    struct framedata
    {
        uint64_t count = 0;
        uint64_t drops = 0;
        uint64_t last_number;
        realdds::dds_nsec total_transit_nsec = 0;
        realdds::dds_nsec max_transit_nsec, min_transit_nsec;
        realdds::dds_time first, last;
        rsutils::number::running_average< dds_nsec > avg_transit_nsec;
    };
    auto process_frame = [time_offset]( std::shared_ptr< framedata > const & fdata,
                                        poc::stream_reader::data_t const & mdata ) {
        auto number = mdata.msg._frame_number;
        //
        // drops
        if( fdata->count && fdata->last_number + 1 != number )
            ++fdata->drops;
        //
        // latency
        auto transit_nsec = mdata.sample.reception_timestamp.to_ns()                  // in our time domain
                          - ( mdata.sample.source_timestamp.to_ns() + time_offset );  // in the embedded time domain
        fdata->avg_transit_nsec.add( transit_nsec );
        fdata->total_transit_nsec += transit_nsec;
        if( ! fdata->count || fdata->max_transit_nsec > transit_nsec )
            fdata->max_transit_nsec = transit_nsec;
        if( ! fdata->count || fdata->min_transit_nsec < transit_nsec )
            fdata->min_transit_nsec = transit_nsec;
        //
        // time spread, so we can average
        fdata->last = realdds::now();
        if( ! fdata->count )
            fdata->first = fdata->last;
        //
        // next
        ++fdata->count;
        fdata->last_number = number;
    };

    using namespace std::placeholders;  // _1, etc...

    auto depth_data = std::make_shared< framedata >();
    {
        poc::stream_reader depth( participant, "depth" );
        depth.on_data( std::bind( process_frame, depth_data, _1 ) );
        depth.wait_for_writers( 1, std::chrono::seconds( 3 ) );

        // Collect frame data
        std::this_thread::sleep_for( std::chrono::seconds( 3 ) );
    }

    // Dump it all out
    LOG_INFO( "Depth:" );
    LOG_INFO( "    in " << timestr(( depth_data->last - depth_data->first ).to_ns(), timestr::abs ) );
    LOG_INFO( "    received " << depth_data->count << " frames" );
    LOG_INFO( "    of which " << depth_data->drops << " frames were dropped" );
    LOG_INFO( "    with an average transit time of " << timestr( depth_data->avg_transit_nsec.get(), timestr::abs ) );

    return EXIT_SUCCESS;
}
catch( const TCLAP::ExitException& )
{
    LOG_ERROR( "Undefined exception while parsing command line arguments" );
    return EXIT_FAILURE;
}
catch( const TCLAP::ArgException& e )
{
    LOG_ERROR( e.what() );
    return EXIT_FAILURE;
}
catch( const std::exception& e )
{
    LOG_ERROR( e.what() );
    return EXIT_FAILURE;
}



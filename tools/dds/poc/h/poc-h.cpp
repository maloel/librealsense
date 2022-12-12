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

enum class stream_enable_flags : uint32_t {
    RGB = 0x1,
    DEPTH = 0x2,
    GYRO = 0x4,
    IMU = 0x8,
    SAFETY = 0x10
};

struct stream_stats_data
{
    uint64_t count = 0;
    uint64_t drops = 0;
    uint64_t last_number;
    realdds::dds_nsec total_transit_nsec = 0;
    realdds::dds_nsec max_transit_nsec, min_transit_nsec;
    realdds::dds_time first, last;
    rsutils::number::running_average< dds_nsec > avg_transit_nsec;
};


static inline bool is_stream_enabled(uint32_t enable_mask, stream_enable_flags stream_bit_flag) {
    uint32_t flag = static_cast<uint32_t>(stream_bit_flag);
    return ((enable_mask & flag) != 0);
}

void stream_stats_data_dump(const string& stream_name, const stream_stats_data& data ) {

    LOG_INFO("stream " << stream_name << " stats:");
    LOG_INFO("  in " << timestr(( data.last - data.first ).to_ns(), timestr::abs ) );
    LOG_INFO("  count: " << data.count);
    LOG_INFO("  drops: " << data.drops);
    LOG_INFO("  last_number: " << data.last_number);
    LOG_INFO("  average transit: " << timestr( depth_data->avg_transit_nsec.get(), timestr::abs ) );
}

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


struct offset_stats {
    dds_nsec avg = 0;
    dds_nsec min = 0;
    dds_nsec max = 0;
    // TODO:
    dds_nsec std = 0;
};

void dump_offset_stats(const offset_stats& data) {
    LOG_INFO( "max-offset= " << timestr(data.max, timestr::rel));
    LOG_INFO( "min-offset= " << timestr(data.min, timestr::rel));
    LOG_INFO( "max-offset-diff= " << timestr(data.max,data.avg));
    LOG_INFO( "min-offset-diff= " << timestr(data.avg, data.min));
    LOG_INFO( "Average time-offset= " << timestr( data.avg, timestr::rel ) );
}

void calc_time_offset(
    poc::op_writer & h2e,
    poc::op_reader & e2h,
    uint32_t const n_reps,
    offset_stats& output_stats)
{
    rsutils::number::running_average< dds_nsec > running_offset_avg;
    dds_nsec max_offset = 0;
    dds_nsec min_offset = 0;
    for( uint64_t i = 0; i < n_reps; ++i )
    {
        auto t0_ = realdds::now().to_ns();
        h2e.write( poc::op_payload::SYNC, i, t0_ );
        auto data = e2h.read( std::chrono::seconds( 300 ) );
        //dds_nsec t0_ = data.msg._data[0];  // before H app send
        dds_nsec t0 = data.msg._data[1];   // "originate" H DDS send time
        dds_nsec t1 = data.msg._data[2];   // "receive" E receive time
        dds_nsec t2 = data.msg._data[3];   // E app send time
        //dds_nsec t2_ = data.sample.source_timestamp.to_ns();  // "transmit" E DDS send time
        dds_nsec t3 = data.sample.reception_timestamp.to_ns();
        dds_nsec t3_ = realdds::now().to_ns();

#if 1
#define RJ(N,S) std::setw(N) << std::right << (S)

        LOG_DEBUG( "\n"
            "    E: " << RJ(45, timestr(t1,timestr::abs,timestr::no_suffix)) << " " << timestr(t2,t1) << "\n"
            "       " << RJ(44, "(" + timestr(t1+running_offset_avg.get(),t0) + ")/" )  << "   \\\n"
            "       " << RJ(43, "/")                          << "     \\\n"
            "       " << RJ(42, "/")                         << "       \\(" << timestr(t3,t2+running_offset_avg.get()) << ")\n"
            "    H: " << RJ(25, timestr(t0_,timestr::no_suffix)) << RJ(16, timestr(t0,t0_) )
                      << "         " << timestr(t3,t0) << "   " << RJ(13, timestr(t3_,t3) ) << "\n"
            );
#else
        LOG_DEBUG( " t0: " << timestr(t0,timestr::abs,timestr::no_suffix));
        LOG_DEBUG( " t1: " << timestr(t1,timestr::abs,timestr::no_suffix));
        LOG_DEBUG( " t2: " << timestr(t2,timestr::abs,timestr::no_suffix));
        LOG_DEBUG( " t3: " << timestr(t3,timestr::abs,timestr::no_suffix));
#endif

        auto time_offset = calc_avg( t0, t3 ) - calc_avg( t1, t2 );
        LOG_DEBUG( "   time-offset= " << timestr( time_offset, timestr::rel ) << "    round-trip= " << timestr( t3_, t0_ ) );
        
        // ignore first iteration
        if( i > 0 ) {
            running_offset_avg.add( time_offset );
            running_offset_avg.add( time_offset );
            if (i== 1) {
                max_offset = time_offset;
                min_offset = time_offset;
            }
            if (time_offset > max_offset) {
                max_offset = time_offset;
            }
            if (time_offset < min_offset) {
                min_offset = time_offset;
            }
		}
    }
    LOG_INFO( "Average time-offset= " << timestr( avg_time_offset, timestr::rel ) );

    output_stats.avg = running_offset_avg.get();
    output_stats.max = max_offset;
    output_stats.min = min_offset;
}


int main( int argc, char** argv ) try
{
    TCLAP::CmdLine cmd( "POC host computer", ' ' );
    TCLAP::SwitchArg debug_arg( "", "debug", "Enable debug logging", false );
    TCLAP::ValueArg< realdds::dds_domain_id > domain_arg( "d", "domain", "select domain ID to listen on", false, 0, "0-232" );
    TCLAP::SwitchArg op_pub_sub_arg( "o", "op-pub-sub", "create op pub and sub", false );
    TCLAP::ValueArg< uint32_t > time_sync_iter_arg( "s", "time-sync", "number of iterations", false, 10, "0-inf" );
    TCLAP::UnlabeledValueArg< string > command_arg( "command", "command to send", false, "", "string" );
    TCLAP::ValueArg< uint32_t > stream_run_time( "t", "run-time", "streaming time in seconds", false, 30, "0-inf" );
    TCLAP::ValueArg< uint32_t > streams_enable_mask(
        "m", "streams-enable-mask", "streams mask to enable", false, static_cast<uint32_t>(stream_enable_flags::DEPTH), "0-inf" );

    cmd.add(domain_arg);
    cmd.add(debug_arg);
    cmd.add(command_arg);
    cmd.add(op_pub_sub_arg);
    cmd.add(time_sync_iter_arg);
    cmd.add(stream_run_time);
    cmd.add(streams_enable_mask);
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

    bool create_op_pub_sub = false;
    if (op_pub_sub_arg.isSet())
    {
        create_op_pub_sub = true;
    }

    auto participant = std::make_shared< realdds::dds_participant >();
    LOG_INFO( "init participant");
    participant->init( domain, "poc-h" );
    dds_nsec time_offset = 0;

    shared_ptr<poc::op_writer> h2e;
    shared_ptr<poc::op_reader> e2h;

    offset_stats start_offset_stats;
    offset_stats end_offset_stats;

    if (create_op_pub_sub) {
        LOG_INFO( "   create h2e writer");
        h2e = make_shared<poc::op_writer>( participant, "realsense/h2e" );
        LOG_INFO( "   h2e writer wait for reader");
        h2e->wait_for_readers( 1, std::chrono::seconds( 300 ) );
        if( command_arg.isSet() )
        {
            if( command_arg.getValue() == "exit" )
            {
                h2e->write( poc::op_payload::EXIT, 0 );
                h2e->wait_for_readers( 0 );
                exit( 0 );
            }
            LOG_ERROR( "Invalid command: " << command_arg.getValue() );
            exit( 1 );
        }

        LOG_INFO( "   create e2h reader");
        e2h = make_shared<poc::op_reader>( participant, "realsense/e2h" );

        if (time_sync_iter_arg.isSet()) {
            calc_time_offset( *h2e, *e2h, time_sync_iter_arg.getValue(), start_offset_stats);
            dump_offset_stats(start_offset_stats);
        }

#if 0
        // Tell E to start
        h2e->write( { { "op", "start" }, { "id", 0 } } );
        auto msg = e2h->read().msg.json_data();  // wait for confirmation
        auto status = utilities::json::get< int64_t >( msg, "status" );
        if( status != 0 )
            LOG_FATAL( "Got bad status " << status << " from E in response to 'start' op" );

#endif
    }
        rsutils::number::running_average< dds_nsec > avg_transit_nsec;

    auto process_frame = [time_offset]( shared_ptr< stream_stats_data > const & fdata,
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
        if( ! fdata->count || fdata->max_transit_nsec < transit_nsec )
            fdata->max_transit_nsec = transit_nsec;
        if( ! fdata->count || fdata->min_transit_nsec > transit_nsec )
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

    using namespace placeholders;  // _1, etc...

    uint8_t num_of_running_streams = 0;

    shared_ptr<poc::stream_reader> depth;
    shared_ptr<poc::stream_reader> rgb;
    shared_ptr<poc::stream_reader> gyro;
    shared_ptr<poc::stream_reader> imu;
    shared_ptr<poc::stream_reader> safety;
    auto depth_data = make_shared< stream_stats_data >();
    auto rgb_data = make_shared< stream_stats_data >();
    auto gyro_data = make_shared< stream_stats_data >();
    auto imu_data = make_shared< stream_stats_data >();
    auto safety_data = make_shared< stream_stats_data >();

    uint8_t streams_mask = streams_enable_mask.getValue();
    LOG_INFO( "   streams_mask: " << hex << streams_mask);
    if (is_stream_enabled(streams_mask, stream_enable_flags::DEPTH)) {
        LOG_INFO( "   create depth stream reader");
        depth = make_shared<poc::stream_reader>( participant, "realsense/depth" );
        num_of_running_streams++;
        depth->on_data( bind( process_frame, depth_data, _1 ));
        depth->wait_for_writers( 1, chrono::seconds( 300 ) );
    }

    if (is_stream_enabled(streams_mask, stream_enable_flags::RGB)) {
        LOG_INFO( "   create rgb stream reader");
        rgb = make_shared<poc::stream_reader>( participant, "realsense/rgb" );
        num_of_running_streams++;
        rgb->on_data( bind( process_frame, rgb_data, _1 ));
        rgb->wait_for_writers( 1, chrono::seconds( 300 ) );
    }

    if (is_stream_enabled(streams_mask, stream_enable_flags::GYRO)) {
        LOG_INFO( "   create gyro stream reader");
        gyro = make_shared<poc::stream_reader>( participant, "realsense/gyro" );
        num_of_running_streams++;
        gyro->on_data( bind( process_frame, gyro_data, _1 ));
        gyro->wait_for_writers( 1, chrono::seconds( 300 ) );
    }

    if (is_stream_enabled(streams_mask, stream_enable_flags::IMU)) {
        LOG_INFO( "   create imu stream reader");
        imu = make_shared<poc::stream_reader>( participant, "realsense/imu" );
        num_of_running_streams++;
        imu->on_data( bind( process_frame, imu_data, _1 ));
        imu->wait_for_writers( 1, chrono::seconds( 300 ) );
    }

    if (is_stream_enabled(streams_mask, stream_enable_flags::SAFETY)) {
        LOG_INFO( "   create safety stream reader");
        safety = make_shared<poc::stream_reader>( participant, "realsense/safety" );
        num_of_running_streams++;
        safety->on_data( bind( process_frame, safety_data, _1 ));
        safety->wait_for_writers( 1, chrono::seconds( 300 ) );
    }

    // Collect frame data
    LOG_INFO( "   streaming should start if enabled");

    LOG_INFO( "   main thread goes to sleep for " << dec << stream_run_time.getValue() << " seconds");
    std::this_thread::sleep_for( std::chrono::seconds( stream_run_time.getValue() ) );

    // Dump it all out somehow

    if (is_stream_enabled(streams_mask, stream_enable_flags::DEPTH))
        stream_stats_data_dump("DEPTH", *depth_data);    

    if (is_stream_enabled(streams_mask, stream_enable_flags::RGB))
        stream_stats_data_dump("RGB", *rgb_data);    

    if (is_stream_enabled(streams_mask, stream_enable_flags::GYRO))
        stream_stats_data_dump("GYRO", *gyro_data);    

    if (is_stream_enabled(streams_mask, stream_enable_flags::IMU))
        stream_stats_data_dump("IMU", *imu_data);    

    if (is_stream_enabled(streams_mask, stream_enable_flags::SAFETY))
        stream_stats_data_dump("SAFETY", *safety_data);    

    if (create_op_pub_sub) {
        if (time_sync_iter_arg.isSet()) {
            calc_time_offset( *h2e, *e2h, time_sync_iter_arg.getValue(), end_offset_stats);
            dump_offset_stats(end_offset_stats);
        }
    }

    LOG_INFO( "extrinsic diff between calculated offsets averages= " << timestr(start_offset_stats.avg, end_offset_stats.avg));
    LOG_INFO( "extrinsic diff between calculated offsets max= " << timestr(start_offset_stats.max, end_offset_stats.max));
    LOG_INFO( "extrinsic diff between calculated offsets min= " << timestr(start_offset_stats.min, end_offset_stats.min));

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



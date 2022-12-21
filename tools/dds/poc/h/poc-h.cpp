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

#ifndef BUILD_SHARED_LIBS
INITIALIZE_EASYLOGGINGPP
#endif


using realdds::dds_nsec;
using realdds::dds_time;
using realdds::timestr;


enum class stream_enable_flags : uint32_t
{
    RGB = 0x1,
    DEPTH = 0x2,
    GYRO = 0x4,
    IMU = 0x8,
    SAFETY = 0x10
};

static inline bool is_stream_enabled( uint32_t enable_mask, stream_enable_flags stream_bit_flag )
{
    uint32_t flag = static_cast<uint32_t>(stream_bit_flag);
    return ((enable_mask & flag) != 0);
}


struct stream_stats_data
{
    uint64_t count = 0;
    uint64_t drops = 0;
    uint64_t last_number;
    realdds::dds_nsec total_transit_nsec = 0;
    realdds::dds_nsec max_transit_nsec, min_transit_nsec;
    realdds::dds_time first, last;
    rsutils::number::running_average< dds_nsec > avg_transit_nsec;

    void dump( const std::string & stream_name ) const
    {
        LOG_INFO( "stream " << stream_name << " stats:" );
        LOG_INFO( "  in " << timestr( (last - first).to_ns(), timestr::abs ) );
        LOG_INFO( "  count: " << count );
        LOG_INFO( "  drops: " << drops );
        LOG_INFO( "  last_number: " << last_number );
        LOG_INFO( "  average transit: " << timestr( avg_transit_nsec.get(), timestr::abs ) );
    }
};


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


struct offset_stats
{
    dds_nsec avg = 0;
    dds_nsec min = 0;
    dds_nsec max = 0;
    // TODO:
    dds_nsec std = 0;

    offset_stats() {}
    offset_stats(dds_nsec avg_, dds_nsec min_, dds_nsec max_) :
        avg(avg_), min(min_), max(max_) {}

    void dump() const
    {
        LOG_INFO( "max-offset= " << timestr( max, timestr::rel ) );
        LOG_INFO( "min-offset= " << timestr( min, timestr::rel ) );
        LOG_INFO( "max-offset-diff= " << timestr( max, avg ) );
        LOG_INFO( "min-offset-diff= " << timestr( avg, min ) );
        LOG_INFO( "Average time-offset= " << timestr( avg, timestr::rel ) );
    }
};


offset_stats calc_time_offset( poc::op_writer & h2e, poc::op_reader & e2h, uint32_t const n_reps )
{
    rsutils::number::running_average< dds_nsec > running_offset_avg;
    dds_nsec max_offset = 0;
    dds_nsec min_offset = 0;
    for( uint64_t i = 0; i < n_reps; ++i )
    {
        auto t0_ = realdds::now().to_ns();
        h2e.write( poc::op_payload::SYNC, i, t0_ );
        auto data = e2h.read( std::chrono::seconds( 300 ) );
        dds_nsec const e_skew = 0; // -5000000000;
        //dds_nsec t0_ = data.msg._data[0];  // before H app send
        dds_nsec t0 = data.msg._data[1];   // "originate" H DDS send time
        dds_nsec t1 = data.msg._data[2] + e_skew;   // "receive" E receive time
        dds_nsec t2 = data.msg._data[3] + e_skew;   // E app send time
        dds_nsec t2_ = data.sample.source_timestamp.to_ns();  // "transmit" E DDS send time
        dds_nsec t3 = data.sample.reception_timestamp.to_ns();
        dds_nsec t3_ = realdds::now().to_ns();


#define RJ(N,S) std::setw(N) << std::right << (S)

        LOG_DEBUG( "\n"
            "    E: " << RJ(45, timestr(t1,timestr::abs,timestr::no_suffix)) << " " << timestr(t2,t1) << "       =" << timestr(t2_,timestr::abs,timestr::no_suffix) << "\n"
            "       " << RJ(44, "(" + timestr(t1+running_offset_avg.get(),t0) + ")/" )  << "   \\\n"
            "       " << RJ(43, "/")                          << "     \\\n"
            "       " << RJ(42, "/")                         << "       \\(" << timestr(t3,t2+running_offset_avg.get()) << ")\n"
            "    H: " << RJ(25, timestr(t0_,timestr::no_suffix)) << RJ(16, timestr(t0,t0_) )
                << "         " << timestr(t3,t0) << "   " << RJ(13, timestr(t3_,t3) ) << "\n"
            );

#if 1
        LOG_DEBUG( " t0: " << timestr(t0,timestr::abs,timestr::no_suffix));
        LOG_DEBUG( " t1: " << timestr(t1,timestr::abs,timestr::no_suffix));
        LOG_DEBUG( " t2: " << timestr(t2,timestr::abs,timestr::no_suffix));
        LOG_DEBUG( " t2_: " << timestr(t2_,timestr::abs,timestr::no_suffix));
        LOG_DEBUG( " t3: " << timestr(t3,timestr::abs,timestr::no_suffix));
#endif

        auto time_offset = calc_avg( t0, t3 ) - calc_avg( t1, t2 );
        LOG_DEBUG( "   time-offset= " << timestr( time_offset, timestr::rel ) << "    round-trip= " << timestr( t3_, t0_ ) );
        
        // ignore first iteration
        if( i > 0 )
        {
            running_offset_avg.add( time_offset );
            if( i == 1 )
                max_offset = min_offset = time_offset;
            else if( time_offset > max_offset )
                max_offset = time_offset;
            else if( time_offset < min_offset )
                min_offset = time_offset;
        }
    }
    return offset_stats(running_offset_avg.get(), min_offset, max_offset);
}


int main( int argc, char** argv ) try
{
    TCLAP::CmdLine cmd( "POC host computer", ' ' );
    TCLAP::SwitchArg debug_arg( "", "debug", "Enable debug logging", false );
    TCLAP::ValueArg< realdds::dds_domain_id > domain_arg( "d", "domain", "select domain ID to listen on", false, 0, "0-232" );
    TCLAP::ValueArg< uint32_t > time_sync_iter_arg( "s", "time-sync", "number of iterations", false, 10, "0-inf" );
    TCLAP::UnlabeledValueArg< std::string > command_arg( "command", "command to send", false, "", "string" );
    TCLAP::ValueArg< uint32_t > stream_run_time( "t", "run-time", "streaming time in seconds", false, 10, "0-inf" );
    TCLAP::ValueArg< uint32_t > streams_enable_mask(
        "m", "streams-enable-mask", "streams mask to enable", false, static_cast<uint32_t>(stream_enable_flags::DEPTH), "0-inf" );

    cmd.add(domain_arg);
    cmd.add(debug_arg);
    cmd.add(command_arg);
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

    auto participant = std::make_shared< realdds::dds_participant >();
    LOG_INFO( "init participant" );
    participant->init( domain, "poc-h" );
    dds_nsec time_offset = 0;

    std::shared_ptr< poc::op_writer > h2e;
    std::shared_ptr< poc::op_reader > e2h;

    offset_stats start_offset_stats;
    offset_stats end_offset_stats;

    auto n_time_syncs = time_sync_iter_arg.getValue();
    if( command_arg.isSet() || n_time_syncs > 0 )
    {
        LOG_INFO( "   create h2e writer" );
        h2e = std::make_shared< poc::op_writer >( participant, "realsense/h2e" );
        LOG_INFO( "   h2e writer wait for reader" );
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

        LOG_INFO( "   create e2h reader" );
        e2h = std::make_shared< poc::op_reader >( participant, "realsense/e2h" );

        if( n_time_syncs > 0 )
        {
            start_offset_stats = calc_time_offset( *h2e, *e2h, n_time_syncs );
            start_offset_stats.dump();
        }
    }

    auto streams_mask = streams_enable_mask.getValue();
    LOG_INFO( "   streams_mask: " << std::hex << streams_mask << std::dec );
    if( ! streams_mask )
        return EXIT_SUCCESS;

    time_offset = start_offset_stats.avg;
    auto process_frame = [time_offset]( std::shared_ptr< stream_stats_data > const & fdata,
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

    using namespace std::placeholders;  // _1, etc...

    std::shared_ptr< stream_stats_data > depth_data, rgb_data, gyro_data, imu_data, safety_data;
    {
        std::shared_ptr< poc::stream_reader > depth;
        if( is_stream_enabled( streams_mask, stream_enable_flags::DEPTH ) )
        {
            LOG_INFO( "   create depth stream reader" );
            depth_data = std::make_shared< stream_stats_data >();
            depth = std::make_shared< poc::stream_reader >( participant, "realsense/depth" );
            depth->on_data( bind( process_frame, depth_data, _1 ) );
            depth->wait_for_writers( 1, std::chrono::seconds( 300 ) );
        }

        std::shared_ptr< poc::stream_reader > rgb;
        if( is_stream_enabled( streams_mask, stream_enable_flags::RGB ) )
        {
            LOG_INFO( "   create rgb stream reader" );
            rgb_data = std::make_shared< stream_stats_data >();
            rgb = std::make_shared< poc::stream_reader >( participant, "realsense/rgb" );
            rgb->on_data( bind( process_frame, rgb_data, _1 ) );
            rgb->wait_for_writers( 1, std::chrono::seconds( 300 ) );
        }

        std::shared_ptr< poc::stream_reader > gyro;
        if( is_stream_enabled( streams_mask, stream_enable_flags::GYRO ) )
        {
            LOG_INFO( "   create gyro stream reader" );
            gyro_data = std::make_shared< stream_stats_data >();
            gyro = std::make_shared< poc::stream_reader >( participant, "realsense/gyro" );
            gyro->on_data( bind( process_frame, gyro_data, _1 ) );
            gyro->wait_for_writers( 1, std::chrono::seconds( 300 ) );
        }

        std::shared_ptr< poc::stream_reader > imu;
        if( is_stream_enabled( streams_mask, stream_enable_flags::IMU ) )
        {
            LOG_INFO( "   create imu stream reader" );
            imu_data = std::make_shared< stream_stats_data >();
            imu = std::make_shared< poc::stream_reader >( participant, "realsense/imu" );
            imu->on_data( bind( process_frame, imu_data, _1 ) );
            imu->wait_for_writers( 1, std::chrono::seconds( 300 ) );
        }

        std::shared_ptr< poc::stream_reader > safety;
        if( is_stream_enabled( streams_mask, stream_enable_flags::SAFETY ) )
        {
            LOG_INFO( "   create safety stream reader" );
            safety_data = std::make_shared< stream_stats_data >();
            safety = std::make_shared< poc::stream_reader >( participant, "realsense/safety" );
            safety->on_data( bind( process_frame, safety_data, _1 ) );
            safety->wait_for_writers( 1, std::chrono::seconds( 300 ) );
        }

        // Collect frame data
        LOG_INFO( "   streaming should start if enabled" );

        LOG_INFO( "   main thread goes to sleep for " << std::dec << stream_run_time.getValue() << " seconds" );
        std::this_thread::sleep_for( std::chrono::seconds( stream_run_time.getValue() ) );
        LOG_INFO( " Woke up after " << stream_run_time.getValue() << " seconds" );
    }

    if( command_arg.isSet() || n_time_syncs > 0)
    {
        end_offset_stats = calc_time_offset( *h2e, *e2h, n_time_syncs );
        end_offset_stats.dump();

        LOG_INFO( "extrinsic diff between calculated offsets averages= " << timestr(start_offset_stats.avg, end_offset_stats.avg));
        LOG_INFO( "extrinsic diff between calculated offsets max= " << timestr(start_offset_stats.max, end_offset_stats.max));
        LOG_INFO( "extrinsic diff between calculated offsets min= " << timestr(start_offset_stats.min, end_offset_stats.min));
    }

    if( depth_data )
        depth_data->dump( "DEPTH" );
    if( rgb_data )
        rgb_data->dump( "RGB" );
    if( gyro_data )
        gyro_data->dump( "GYRO" );
    if( imu_data )
        imu_data->dump( "IMU" );
    if( safety_data )
        safety_data->dump( "SAFETY" );

    LOG_INFO( "Done POC ");
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



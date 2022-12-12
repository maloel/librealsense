// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include "streamer.h"

#include <realdds/dds-topic-writer.h>
#include <realdds/dds-topic.h>

#include <fastdds/dds/topic/Topic.hpp>

#include <librealsense2/utilities/easylogging/easyloggingpp.h>


namespace poc {


streamer::streamer( std::shared_ptr< realdds::dds_topic > const & topic, uint64_t id )
    : _writer( std::make_shared< realdds::dds_topic_writer >( topic ) )
    , _id( id )
{
    _writer->on_publication_matched( [this]( eprosima::fastdds::dds::PublicationMatchedStatus const & status ) {
        LOG_DEBUG( name() << ".on_publication_matched " << ( status.current_count_change > 0 ? "+" : "" )
                          << status.current_count_change << " -> " << status.current_count );
        if( status.current_count )
            start_streaming();
        else
            stop_streaming();
    } );
    
    realdds::dds_topic_writer::qos qos(eprosima::fastdds::dds::BEST_EFFORT_RELIABILITY_QOS);
    _writer->run(qos);
}


std::string const & streamer::name() const
{
    return _writer->topic()->get()->get_name();
}


void streamer::start_streaming()
{
    if( _thread.joinable() )
        return;

    LOG_DEBUG( "starting streaming" );
    _is_streaming = true;
    _thread = std::thread( [this]() {
        try
        {
            std::vector< uint8_t > data;
            data.resize( 2048 );
            for( int i = 0; i < data.size(); ++i )
                data[i] = i;
            int64_t number = 0;
            while( _is_streaming )
            {
                poc::stream_payload msg;
                msg._stream_id = _id;
                msg._frame_number = number++;
                msg._data = data;
                msg.write_to( *_writer );
                std::this_thread::sleep_for( std::chrono::milliseconds( 33 ) );
            }
        }
        catch( std::exception & e )
        {
            LOG_ERROR( "error: " << e.what() );
        }
        LOG_DEBUG( "thread stopping" );
    } );
}


void streamer::stop_streaming()
{
    if( ! _thread.joinable() )
        return;

    LOG_DEBUG( "stopping streaming" );
    _is_streaming = false;
    _thread.join();
}


}  // namespace poc

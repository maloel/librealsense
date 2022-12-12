// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once


#include "stream-payload.h"

#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <librealsense2/utilities/concurrency/concurrency.h>

#include <string>
#include <memory>
#include <chrono>
#include <queue>
#include <mutex>
#include <condition_variable>


namespace eprosima {
namespace fastdds {
namespace dds {
struct SubscriptionMatchedStatus;
}
}  // namespace fastdds
}  // namespace eprosima


namespace realdds {
class dds_participant;
class dds_topic;
class dds_topic_reader;
}


namespace poc {


// Ease-of-use helper, easily taking care of basic use-cases that just want to easily read from some topic.
// E.g.:
//     flexible_reader topic( participant, "topic-name" );
//     ...
//     auto msg = topic.read().msg;
//
class stream_reader
{
    std::shared_ptr< realdds::dds_topic_reader > _reader;
    int _n_writers = 0;

public:
    // We need to return both a message and the sample that goes with it, if needed
    struct data_t
    {
        stream_payload msg;
        eprosima::fastdds::dds::SampleInfo sample;
    };
    typedef std::function< void( data_t & ) > on_data_callback;

private:
    std::queue< data_t > _data;
    std::condition_variable _data_cv;
    std::mutex _data_mutex;
    on_data_callback _on_data;

    active_object<> _th;

public:
    stream_reader( std::shared_ptr< realdds::dds_topic > const & topic );
    stream_reader( std::shared_ptr< realdds::dds_participant > const & participant, std::string const & topic_name )
        : stream_reader( stream_payload::create_topic( participant, topic_name ) )
    {
    }

    std::string const & name() const;

    void wait_for_writers( int n_writers, std::chrono::seconds timeout );

    void on_data( on_data_callback callback ) { _on_data = std::move( callback ); }

    // Block until data is available
    void wait_for_data();
    // Block, but throw on timeout
    void wait_for_data( std::chrono::seconds timeout );

    // Blocking -- waits until data is available
    data_t read();
    // Blocking -- but with a timeout (throws)
    data_t read( std::chrono::seconds timeout );

    bool empty() const { return _data.empty(); }

private:
    void on_subscription_matched( eprosima::fastdds::dds::SubscriptionMatchedStatus const & );
    void on_data_available();
    data_t pop_data();
};


}  // namespace poc

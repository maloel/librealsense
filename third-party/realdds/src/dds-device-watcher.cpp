// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/dds-device-watcher.h>
#include <realdds/dds-topic.h>
#include <realdds/dds-topic-reader-thread.h>
#include <realdds/dds-device.h>
#include <realdds/dds-utilities.h>
#include <realdds/topics/dds-topic-names.h>
#include <realdds/topics/flexible-msg.h>
#include <realdds/topics/device-info-msg.h>

#include <rsutils/json.h>

#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>


using namespace eprosima::fastdds::dds;
using namespace realdds;


dds_device_watcher::dds_device_watcher( std::shared_ptr< dds_participant > const & participant )
    : _device_info_topic(
        new dds_topic_reader_thread( topics::flexible_msg::create_topic( participant, topics::DEVICE_INFO_TOPIC_NAME ) ) )
    , _participant( participant )
{
    _device_info_topic->on_data_available(
        [this]()
        {
            topics::flexible_msg msg;
            eprosima::fastdds::dds::SampleInfo info;
            while( topics::flexible_msg::take_next( *_device_info_topic, &msg, &info ) )
            {
                if( ! msg.is_valid() )
                    continue;

                // NOTE: the GUID we get here is the writer on the device-info, used nowhere again in the system, which
                // can therefore be confusing. It is not the "server GUID" that's stored in the device!
                dds_guid guid;
                eprosima::fastrtps::rtps::iHandle2GUID( guid, info.publication_handle );

                auto const j = msg.json_data();

                std::string root;
                if( ! rsutils::json::get_ex( j, "topic-root", &root ) )
                {
                    // A topic-root is required, as it uniquely identifies the device across GUIDs, participants, etc.
                    LOG_DEBUG( "device-info from " << _participant->print( guid ) << " is missing a topic-root; ignoring: " << j );
                    continue;
                }
                bool stopping = ( j.find( "stopping" ) != j.end() );

                {
                    std::lock_guard< std::mutex > lock( _devices_mutex );
                    auto it = _root_liveliness.find( root );
                    if( it != _root_liveliness.end() )
                    {
                        auto & is = it->second;
                        is.last_seen = now();
                        if( stopping )
                        {
                            // We marked last-seen; nothing else to do with it
                        }
                        else if( is.alive )
                        {
                            // We already know about this device; likely this was a broadcast meant for someone else
                            continue;
                        }
                        else if( is.alive = is.in_use.lock() )
                        {
                            // Old device coming back to life
                            is.writer_guid = guid;
                            LOG_DEBUG( "DDS device (from " << _participant->print( guid ) << ") back to life: " << j.dump( 4 ) );
                            topics::device_info device_info = topics::device_info::from_json( j );
                            static_cast< dds_discovery_sink * >( is.alive.get() )->on_discovery_restored( device_info );
                            if( _on_device_added )
                            {
                                std::thread( [device = is.alive, on_device_added = _on_device_added]()
                                             { on_device_added( device ); } )
                                    .detach();
                            }
                            continue;
                        }
                        else
                        {
                            // Old device that's popped back up; recreate it
                        }
                    }
                }

                if( stopping )
                {
                    // This device is stopping for whatever reason (e.g., HW reset); remove it
                    LOG_DEBUG( "DDS device (from " << _participant->print( guid ) << ") is stopping: " << root );
                    // TODO notify the device?
                    remove_device( root );
                    continue;
                }

                LOG_DEBUG( "DDS device (from " << _participant->print( guid ) << ") detected: " << j.dump( 4 ) );
                topics::device_info device_info = topics::device_info::from_json( j );

                // Add a new device record into our dds devices map
                std::shared_ptr< dds_device > device = std::make_shared< dds_device >( _participant, device_info );
                {
                    std::lock_guard< std::mutex > lock( _devices_mutex );
                    auto & is = _root_liveliness[root];
                    is.alive = device;
                    is.writer_guid = guid;
                    is.last_seen = now();
                }

                // NOTE: device removals are handled via the writer-removed notification; see on_subscription_matched() below
                if( _on_device_added )
                {
                    std::thread(
                        [device, on_device_added = _on_device_added]() {  //
                            on_device_added( device );
                        } )
                        .detach();
                }
            }
        } );

    _device_info_topic->on_subscription_matched(
        [this]( eprosima::fastdds::dds::SubscriptionMatchedStatus const & status )
        {
            if( status.current_count_change == -1 )
            {
                dds_guid const guid
                    = status.last_publication_handle.operator const eprosima::fastrtps::rtps::GUID_t &();
                liveliness_map::const_iterator it;
                {
                    std::lock_guard< std::mutex > lock( _devices_mutex );
                    it = std::find_if( _root_liveliness.begin(),
                                       _root_liveliness.end(),
                                       [guid]( liveliness_map::value_type const & it )
                                       { return it.second.writer_guid == guid; } );
                    if( it == _root_liveliness.end() )
                        // This is OK, and is likely the broadcaster's writer itself; ignore
                        return;
                }
                LOG_DEBUG( "DDS device (from " << _participant->print( guid ) << ") disconnected: " << it->first );
                remove_device( it->first );
            }
        } );

    if( ! _participant->is_valid() )
        DDS_THROW( runtime_error, "participant was not initialized" );
}


bool dds_device_watcher::is_stopped() const
{
    return ! _device_info_topic->is_running();
}


void dds_device_watcher::start()
{
    stop();
    if( ! _device_info_topic->is_running() )
        init();
    LOG_DEBUG( "DDS device watcher started on '" << _participant->get()->get_qos().name() << "' "
                                                 << realdds::print( _participant->guid() ) );
}

void dds_device_watcher::stop()
{
    if( ! is_stopped() )
    {
        _device_info_topic->stop();
        //_callback_inflight.wait_until_empty();
        LOG_DEBUG( "DDS device watcher stopped" );
    }
}


dds_device_watcher::~dds_device_watcher()
{
    stop();
}


void dds_device_watcher::init()
{
    if( ! _device_info_topic->is_running() )
        _device_info_topic->run( dds_topic_reader::qos() );
}


void dds_device_watcher::remove_device( std::string const & root )
{
    std::shared_ptr< dds_device > device;
    {
        std::lock_guard< std::mutex > lock( _devices_mutex );
        auto it = _root_liveliness.find( root );
        if( it == _root_liveliness.end() )
            return;
        auto & is = it->second;
        device = is.alive;
        if( ! device )
            return;
        static_cast< dds_discovery_sink * >( device.get() )->on_discovery_lost();
        is.in_use = is.alive;
        is.alive.reset();  // no longer alive; in_use will track whether it's being used
    }
    // rest must happen outside the mutex
    std::thread(
        [device, on_device_removed = _on_device_removed]()
        {
            if( on_device_removed )
                on_device_removed( device );
            // If we're holding the device, it will get destroyed here, from another thread.
            // Not sure why, but if we delete the outside this thread (in the listener callback), it
            // will cause some sort of invalid state in DDS. The thread will get killed and we won't get
            // any notification of the remote participant getting removed... and the process will even
            // hang on exit.
        } )
        .detach();
}


bool dds_device_watcher::foreach_device(
    std::function< bool( std::shared_ptr< dds_device > const & ) > fn ) const
{
    std::lock_guard< std::mutex > lock( _devices_mutex );
    for( auto & root_liveliness : _root_liveliness )
    {
        auto & is = root_liveliness.second;
        if( is.alive )
            if( ! fn( is.alive ) )
                return false;
    }
    return true;
}


bool dds_device_watcher::is_device_broadcast( std::shared_ptr< dds_device > const & dev ) const
{
    auto & root = dev->device_info().topic_root;
    std::lock_guard< std::mutex > lock( _devices_mutex );
    auto it = _root_liveliness.find( root );
    if( it == _root_liveliness.end() )
        return false;
    return ! ! it->second.alive;
}


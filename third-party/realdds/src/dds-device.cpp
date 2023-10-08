// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/dds-device.h>
#include <realdds/dds-participant.h>
#include <realdds/dds-topic-reader.h>
#include <realdds/dds-topic-writer.h>
#include "dds-device-impl.h"

#include <rsutils/time/timer.h>
#include <rsutils/json.h>


namespace realdds {


dds_device::dds_device( std::shared_ptr< dds_participant > const & participant, topics::device_info const & info )
    : _impl( std::make_shared< dds_device::impl >( participant, info ) )
{
    LOG_DEBUG( "+device '" << _impl->debug_name() << "' on " << info.topic_root() );
}


bool dds_device::is_ready() const
{
    return _impl->is_ready();
}


void dds_device::wait_until_ready( size_t timeout_ms )
{
    if( is_ready() )
        return;

    LOG_DEBUG( "waiting for '" << device_info().debug_name() << "' ..." );
    rsutils::time::timer timer{ std::chrono::milliseconds( timeout_ms ) };
    bool was_online = is_online();
    do
    {
        if( timer.has_expired() )
            DDS_THROW( runtime_error, "timeout waiting for '" << device_info().debug_name() << "'" );
        std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
        if( was_online )
        {
            if( ! is_online() )
                DDS_THROW( runtime_error, "device went offline '" << device_info().debug_name() << "'" );
        }
        else
            was_online = is_online();
    }
    while( ! is_ready() );
}


bool dds_device::is_online() const
{
    if( _impl->_lost_discovery )
        return false;
    if( ! _impl->_notifications_reader->has_writers() )
        return false;
    if( ! _impl->_control_writer->has_readers() )
        return false;
    return true;
}


void dds_device::wait_until_online( size_t timeout_ms )
{
    if( is_online() )
        return;

    LOG_DEBUG( "waiting for '" << device_info().debug_name() << "' to come online ..." );
    rsutils::time::timer timer{ std::chrono::milliseconds( timeout_ms ) };
    do
    {
        if( timer.has_expired() )
            DDS_THROW( runtime_error, "timeout waiting for '" << device_info().debug_name() << "' to come online" );
        std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
    }
    while( ! is_online() );
}


void dds_device::on_discovery_lost()
{
    // Called when the device-watcher has lost connection with the device
    // Only devices that are discovered by the device-watcher get called with this!
    _impl->_lost_discovery = true;
    _impl->set_state( impl::state_t::WAIT_FOR_DEVICE_HEADER );
}


void dds_device::on_discovery_restored( topics::device_info const & new_info )
{
    // Called when the device-watcher has re-connected with a device that was lost before
    // Only devices that are discovered by the device-watcher get called with this!
    if( new_info.name != device_info().name )
        DDS_THROW( runtime_error, "device name cannot change" );
    if( new_info.topic_root != device_info().topic_root )
        DDS_THROW( runtime_error, "topic root cannot change" );
    if( new_info.serial != device_info().serial )
        DDS_THROW( runtime_error, "device serial number cannot change" );
    if( new_info.product_line != device_info().product_line )
        DDS_THROW( runtime_error, "product line cannot change" );

    _impl->_info = new_info;
    _impl->_lost_discovery = false;
}


std::shared_ptr< dds_participant > const& dds_device::participant() const
{
    return _impl->_participant;
}

std::shared_ptr< dds_subscriber > const & dds_device::subscriber() const
{
    return _impl->_subscriber;
}

topics::device_info const & dds_device::device_info() const
{
    return _impl->_info;
}

dds_guid const & dds_device::server_guid() const
{
    return _impl->_server_guid;
}

dds_guid const & dds_device::guid() const
{
    return _impl->guid();
}

size_t dds_device::number_of_streams() const
{
    return _impl->_streams.size();
}

size_t dds_device::foreach_stream( std::function< void( std::shared_ptr< dds_stream > stream ) > fn ) const
{
    for ( auto const & stream : _impl->_streams )
    {
        fn( stream.second );
    }

    return _impl->_streams.size();
}

size_t dds_device::foreach_option( std::function< void( std::shared_ptr< dds_option > option ) > fn ) const
{
    for( auto const & option : _impl->_options)
    {
        fn( option );
    }

    return _impl->_options.size();
}

void dds_device::open( const dds_stream_profiles & profiles )
{
    wait_until_online();
    _impl->open( profiles );
}

void dds_device::set_option_value( const std::shared_ptr< dds_option > & option, float new_value )
{
    wait_until_online();
    _impl->set_option_value( option, new_value );
}

float dds_device::query_option_value( const std::shared_ptr< dds_option > & option )
{
    wait_until_online();
    return _impl->query_option_value( option );
}

void dds_device::send_control( topics::flexible_msg && msg, rsutils::json * reply )
{
    wait_until_online();
    _impl->write_control_message( std::move( msg ), reply );
}

bool dds_device::has_extrinsics() const
{
    return ! _impl->_extrinsics_map.empty();
}

std::shared_ptr< extrinsics > dds_device::get_extrinsics( std::string const & from, std::string const & to ) const
{
    auto iter = _impl->_extrinsics_map.find( std::make_pair( from, to ) );
    if( iter != _impl->_extrinsics_map.end() )
        return iter->second;

    std::shared_ptr< extrinsics > empty;
    return empty;
}

bool dds_device::supports_metadata() const
{
    return !! _impl->_metadata_reader;
}

rsutils::subscription dds_device::on_metadata_available( on_metadata_available_callback && cb )
{
    return _impl->on_metadata_available( std::move( cb ) );
}

rsutils::subscription dds_device::on_device_log( on_device_log_callback && cb )
{
    return _impl->on_device_log( std::move( cb ) );
}

rsutils::subscription dds_device::on_notification( on_notification_callback && cb )
{
    return _impl->on_notification( std::move( cb ) );
}


static std::string const status_key( "status", 6 );
static std::string const status_ok( "ok", 2 );
static std::string const explanation_key( "explanation", 11 );
static std::string const id_key( "id", 2 );


bool dds_device::check_reply( rsutils::json const & reply, std::string * p_explanation )
{
    auto status_j = reply.nested( status_key );
    if( ! status_j )
        return true;
    std::ostringstream os;
    if( ! status_j.is_string() )
        os << "bad status " << status_j;
    else if( status_j.string_ref() == status_ok )
        return true;
    else
    {
        os << "[";
        if( auto id = reply.nested( id_key ) )
        {
            if( id.is_string() )
                os << "\"" << id.string_ref() << "\" ";
        }
        os << status_j.string_ref() << "]";
        if( auto explanation_j = reply.nested( explanation_key ) )
        {
            os << ' ';
            if( explanation_j.string_ref_or_empty().empty() )
                os << "bad explanation " << explanation_j;
            else
                os << explanation_j.string_ref();
        }
    }
    if( ! p_explanation )
        DDS_THROW( runtime_error, os.str() );
    *p_explanation = os.str();
    return false;
}


}  // namespace realdds

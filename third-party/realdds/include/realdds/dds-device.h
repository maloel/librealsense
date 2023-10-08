// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.
#pragma once

#include "dds-defines.h"
#include "dds-stream-profile.h"
#include "dds-stream.h"
#include "dds-discovery-sink.h"

#include <rsutils/subscription.h>
#include <rsutils/json-fwd.h>
#include <memory>
#include <vector>
#include <functional>
#include <string>

namespace realdds {

namespace topics {
class device_info;
}  // namespace topics


class dds_participant;

// Represents a device via the DDS system. Such a device exists as of its identification by the device-watcher, and
// always contains a device-info.
// 
// The device may not be ready for use (will not contain sensors, profiles, etc.) until it receives all handshake
// notifications from the server (see docs/initialization.md), but it will start receiving notifications and be able to
// send controls right away, as long as it is online. If a device is part of discovery (owned by a device-watcher), then
// lost discovery will cause the device to lose its ready state.
//
class dds_device : public dds_discovery_sink
{
public:
    dds_device( std::shared_ptr< dds_participant > const &, topics::device_info const & );

    std::shared_ptr< dds_participant > const & participant() const;
    std::shared_ptr< dds_subscriber > const & subscriber() const;
    topics::device_info const & device_info() const;

    dds_guid const & server_guid() const;  // server notification writer
    dds_guid const & guid() const;         // client control writer (and notification samples)

    // A device is ready for use after it's gone through handshake and can start streaming.
    // Losing discovery will lose ready status.
    bool is_ready() const;

    // Wait until ready. Will throw if not ready within the timeout!
    void wait_until_ready( size_t timeout_ms = 5000 );

    // A device is offline when there's nobody to talk to: nobody to listen or send controls to
    bool is_online() const;

    // Throws if not online within the timeout!
    void wait_until_online( size_t timeout_ms = 5000 );

    // Utility function for checking replies:
    // If 'p_explanation' is nullptr, will throw if the reply status is not 'ok'.
    // Otherise will return a false if not 'ok', and the explanation will be filled out.
    static bool check_reply( rsutils::json const & reply, std::string * p_explanation = nullptr );

    //----------- below this line, a device must be ready!

    size_t number_of_streams() const;

    size_t foreach_stream( std::function< void( std::shared_ptr< dds_stream > stream ) > fn ) const;
    size_t foreach_option( std::function< void( std::shared_ptr< dds_option > option ) > fn ) const;

    void open( const dds_stream_profiles & profiles );

    void set_option_value( const std::shared_ptr< dds_option > & option, float new_value );
    float query_option_value( const std::shared_ptr< dds_option > & option );

    void send_control( topics::flexible_msg &&, rsutils::json * reply = nullptr );

    bool has_extrinsics() const;
    std::shared_ptr< extrinsics > get_extrinsics( std::string const & from, std::string const & to ) const;

    bool supports_metadata() const;

    typedef std::function< void( std::shared_ptr< const rsutils::json > const & md ) > on_metadata_available_callback;
    rsutils::subscription on_metadata_available( on_metadata_available_callback && );

    typedef std::function< void(
        dds_nsec timestamp, char type, std::string const & text, rsutils::json const & data ) >
        on_device_log_callback;
    rsutils::subscription on_device_log( on_device_log_callback && cb );

    typedef std::function< void( std::string const & id, rsutils::json const & ) > on_notification_callback;
    rsutils::subscription on_notification( on_notification_callback && );

    // dds_discovery_sink
protected:
    void on_discovery_lost() override;
    void on_discovery_restored( topics::device_info const & ) override;

private:
    class impl;
    std::shared_ptr< impl > _impl;
};


}  // namespace realdds

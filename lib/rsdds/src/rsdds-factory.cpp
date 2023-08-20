// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include <rsutils/easylogging/easyloggingpp.h>

#include <realdds/dds-device-watcher.h>
#include <realdds/dds-participant.h>
#include <realdds/dds-device.h>
#include <realdds/topics/device-info-msg.h>

#include <rsutils/shared-ptr-singleton.h>
#include <rsutils/os/executable-name.h>
#include <rsutils/string/from.h>

#include <rscore/factory-registry.h>
#include <rscore/device-factory.h>
#include <rscore/device-info.h>
#include <rsutils/json.h>

#include "rs-dds-device-info.h"


// We manage one participant and device-watcher per domain:
// Two contexts with the same domain-id will share the same participant and watcher, while a third context on a
// different domain will have its own.
//
struct dds_domain_context
{
    rsutils::shared_ptr_singleton< realdds::dds_participant > participant;
    rsutils::shared_ptr_singleton< realdds::dds_device_watcher > device_watcher;
};
//
// Domains are mapped by ID:
// Two contexts with the same participant name on different domain-ids are using two different participants!
//
static std::map< realdds::dds_domain_id, dds_domain_context > dds_domain_context_by_id;


namespace librealsense {


class rs_dds_device_factory : public device_factory
{
    std::shared_ptr< realdds::dds_device_watcher > _dds_watcher;
    std::shared_ptr< context > _context;

public:
    rs_dds_device_factory( std::shared_ptr< context > const & ctx,
                           std::shared_ptr< realdds::dds_device_watcher > const & device_watcher )
        : _dds_watcher( device_watcher )
    {
    }

    void query_devices( device_info_callback callback ) override
    {
        if( ! _dds_watcher )
            return;

        _dds_watcher->foreach_device(
            [&]( std::shared_ptr< realdds::dds_device > const & dev ) -> bool
            {
                if( !dev->is_ready() )
                {
                    LOG_DEBUG( "device '" << dev->device_info().debug_name() << "' is not yet ready" );
                    return true;
                }

                std::shared_ptr< device_info > dev_info = std::make_shared< dds_device_info >( _context, dev );
                return callback( dev_info );
            } );
    }
};


class rs_dds_core_factory : public rscore_factory
{
    typedef rscore_factory super;

    std::shared_ptr< realdds::dds_participant > _dds_participant;
    std::shared_ptr< realdds::dds_device_watcher > _dds_watcher;

    std::shared_ptr< context > _context;

    device_changed_callback _on_device_added;
    device_changed_callback _on_device_removed;

public:
    rs_dds_core_factory( std::string const & name )
        : super( name )
    {
    }

    bool initialize_factory( std::shared_ptr< context > const & ctx, nlohmann::json const & settings ) override
    {
        _context = ctx;

        nlohmann::json dds_settings
            = rsutils::json::get< nlohmann::json >( settings, std::string( "dds", 3 ), nlohmann::json::object() );
        if( dds_settings.is_object() )
        {
            realdds::dds_domain_id domain_id = rsutils::json::get< int >( dds_settings, std::string( "domain", 6 ), 0 );
            std::string participant_name = rsutils::json::get< std::string >( dds_settings,
                                                                              std::string( "participant", 11 ),
                                                                              rsutils::os::executable_name() );

            auto & domain = dds_domain_context_by_id[domain_id];
            _dds_participant = domain.participant.instance();
            if( ! _dds_participant->is_valid() )
            {
                _dds_participant->init( domain_id, participant_name, std::move( dds_settings ) );
            }
            else if( rsutils::json::has_value( dds_settings, std::string( "participant", 11 ) )
                     && participant_name != _dds_participant->name() )
            {
                throw std::runtime_error( rsutils::string::from() << "A DDS participant '" << _dds_participant->name()
                                                                  << "' already exists in domain " << domain_id
                                                                  << "; cannot create '" << participant_name << "'" );
            }
            _dds_watcher = domain.device_watcher.instance( _dds_participant );

            // The DDS device watcher should always be on
            if( _dds_watcher && _dds_watcher->is_stopped() )
            {
                start_dds_device_watcher();
            }
        }
        return true;
    }

    void on_device_added( device_changed_callback callback ) override
    {
        _on_device_added = callback;
    }

    void on_device_removed( device_changed_callback callback ) override
    {
        _on_device_removed = callback;
    }

    void start_dds_device_watcher()
    {
        _dds_watcher->on_device_added(
            [this]( std::shared_ptr< realdds::dds_device > const & dev )
            {
                if( _on_device_added )
                {
                    dev->wait_until_ready();  // make sure handshake is complete
                    _on_device_added( std::make_shared< dds_device_info >( _context, dev ) );
                }
            } );
        _dds_watcher->on_device_removed(
            [this]( std::shared_ptr< realdds::dds_device > const & dev )
            {
                if( _on_device_removed )
                    _on_device_removed( std::make_shared< dds_device_info >( _context, dev ) );
            } );
        _dds_watcher->start();
    }

    std::shared_ptr< device_factory > get_device_factory() override
    {
        return std::make_shared< rs_dds_device_factory >( _context, _dds_watcher );
    }
};


REGISTER_RSCORE_FACTORY( rs_dds_core_factory, "dds" );


}  // namespace librealsense
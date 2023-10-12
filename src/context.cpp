// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <rscore/context.h>
#include "device-info.h"

#include "backend-device-factory.h"
#ifdef BUILD_WITH_DDS
#include "dds/rsdds-device-factory.h"
#endif

#include <rscore/module-registry.h>
#include <rscore/context-module.h>
#include <librealsense2/hpp/rs_types.hpp>

#include <librealsense2/hpp/rs_types.hpp>  // rs2_devices_changed_callback
#include <librealsense2/rs.h>              // RS2_API_VERSION_STR
#include <src/librealsense-exception.h>

#include <rsutils/easylogging/easyloggingpp.h>
#include <rsutils/string/from.h>
#include <rsutils/json.h>
using json = nlohmann::json;


namespace librealsense
{
    context::context( json const & settings )
        : _settings( settings )
        , _device_mask( rsutils::json::get< unsigned >( settings, "device-mask", RS2_PRODUCT_LINE_ANY ) )
    {
        static bool version_logged = false;
        if( ! version_logged )
        {
            version_logged = true;
            LOG_DEBUG( "Librealsense VERSION: " << RS2_API_VERSION_STR );
        }

        _device_factories.push_back( std::make_shared< backend_device_factory >(
            *this,
            [this]( std::vector< std::shared_ptr< device_info > > const & removed,
                    std::vector< std::shared_ptr< device_info > > const & added )
            { invoke_devices_changed_callbacks( removed, added ); } ) );

#ifdef BUILD_WITH_DDS
        _device_factories.push_back( std::make_shared< rsdds_device_factory >(
            *this,
            [this]( std::vector< std::shared_ptr< device_info > > const & removed,
                    std::vector< std::shared_ptr< device_info > > const & added )
            { invoke_devices_changed_callbacks( removed, added ); } ) );
#endif

        _modules = module_registry::create_context_modules( *this );
        for( auto & m : _modules )
        {
            auto d_factory = m->create_device_factory(
                [this]( std::vector< rs2_device_info > const & removed, std::vector< rs2_device_info > const & added )
                { invoke_devices_changed_callbacks( removed, added ); } );
            if( d_factory )
                _device_factories.push_back( d_factory );
        }
    }


    context::context( char const * json_settings )
        : context( json_settings ? json::parse( json_settings ) : json() )
    {
    }


    context::~context()
    {
    }


    /*static*/ unsigned context::combine_device_masks( unsigned const requested_mask, unsigned const mask_in_settings )
    {
        unsigned mask = requested_mask;
        // The normal bits enable, so enable only those that are on
        mask &= mask_in_settings & ~RS2_PRODUCT_LINE_SW_ONLY;
        // But the above turned off the SW-only bits, so turn them back on again
        if( ( mask_in_settings & RS2_PRODUCT_LINE_SW_ONLY ) || ( requested_mask & RS2_PRODUCT_LINE_SW_ONLY ) )
            mask |= RS2_PRODUCT_LINE_SW_ONLY;
        return mask;
    }


    std::vector< std::shared_ptr< device_info > > context::query_devices( int requested_mask ) const
    {
        std::vector< std::shared_ptr< device_info > > list;
        for( auto & factory : _device_factories )
        {
            for( auto & dev_info : factory->query_devices( requested_mask ) )
            {
                LOG_INFO( "... " << dev_info->get_address() );
                list.push_back( dev_info );
            }
        }
        for( auto & item : _user_devices )
        {
            if( auto dev_info = item.second.lock() )
            {
                LOG_INFO( "... " << dev_info->get_address() );
                list.push_back( dev_info );
            }
        }
        LOG_INFO( "Found " << list.size() << " RealSense devices (0x" << std::hex << requested_mask << " requested & 0x"
                           << get_device_mask() << " from device-mask in settings)" << std::dec );
        return list;
    }


    void context::invoke_devices_changed_callbacks(
        std::vector< std::shared_ptr< device_info > > const & rs2_devices_info_removed,
        std::vector< std::shared_ptr< device_info > > const & rs2_devices_info_added )
    {
        _devices_changed.raise( rs2_devices_info_removed, rs2_devices_info_added );
    }


    rsutils::subscription context::on_device_changes( devices_changed_callback && callback )
    {
        return _devices_changed.subscribe( std::move( callback ) );
    }


    void context::add_device( std::shared_ptr< device_info > const & dev )
    {
        auto address = dev->get_address();

        auto it = _user_devices.find( address );
        if( it != _user_devices.end() && it->second.lock() )
            throw librealsense::invalid_value_exception( "device already in context: " + address );
        _user_devices[address] = dev;

        std::vector< std::shared_ptr< device_info > > rs2_device_info_added{ dev };
        invoke_devices_changed_callbacks( {}, rs2_device_info_added );
    }


    void context::remove_device( std::shared_ptr< device_info > const & dev )
    {
        auto address = dev->get_address();

        auto it = _user_devices.find( address );
        if(it == _user_devices.end() )
            return;  // Why not throw?!
        auto dev_info = it->second.lock();
        _user_devices.erase(it);

        if( dev_info )
        {
            std::vector< std::shared_ptr< device_info > > rs2_device_info_removed{ dev_info };
            invoke_devices_changed_callbacks( rs2_device_info_removed, {} );
        }
    }

}

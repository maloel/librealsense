// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#include "rsdds-options-watcher.h"
#include <src/core/options-registry.h>
#include <realdds/dds-device.h>
#include <realdds/topics/flexible-msg.h>
#include <rsutils/json.h>
#include <rsutils/easylogging/easyloggingpp.h>

using rsutils::json;


namespace librealsense {


dds_options_watcher::dds_options_watcher( std::shared_ptr< realdds::dds_device > const & device,
                                          std::string const & sensor_name )
    : _dds_dev( device )
    , _sensor_name( sensor_name )
{
}


dds_options_watcher::options_and_values dds_options_watcher::update_options()
{
    realdds::topics::flexible_msg msg( json{
        { "id", "query-option" },
        { "sensor-name", _sensor_name },
        { "option-name", json::array() }  // [] = empty array = get all option values
    } );

    rsutils::json reply;
    _dds_dev->send_control( std::move( msg ), &reply );

    if( auto options = reply.nested( "option-values", &json::is_array ) )
    {
        for( size_t x = 0; x < options.size(); ++x )
        {
            auto & option = options[x];
            if( ! option.is_array() )
            {
                LOG_DEBUG( "[option " << x << "] not an array: " << option );
                continue;
            }
            if( option.size() == 2 )
            {
                auto option_name = option[0].string_ref_or_empty();
                if( option_name.empty() )
                {
                    LOG_DEBUG( "[option " << x << "] invalid name: " << option );
                    continue;
                }
                auto & value = option[1];
            }
            else if( option.size() == 3 )
            {
                auto stream_name = option[0].string_ref_or_empty();
                if( stream_name.empty() )
                {
                    LOG_DEBUG( "[option " << x << "] invalid stream: " << option );
                    continue;
                }
                auto option_name = option[1].string_ref_or_empty();
                if( option_name.empty() )
                {
                    LOG_DEBUG( "[option " << x << "] invalid name: " << option );
                    continue;
                }
                auto & value = option[2];
            }
            else
            {
            }
        }
    }
    else
        throw std::runtime_error( "" );


    options_and_values updated_options;
#if 0
    std::shared_ptr< raw_sensor_base > strong = _raw_sensor.lock();
    if( ! strong )
        return updated_options;
    try
    {
        strong->prepare_for_bulk_operation();
        updated_options = options_watcher::update_options();
        strong->finished_bulk_operation();
    }
    catch( const std::exception & ex )
    {
        LOG_ERROR( "Error when updating options: " << ex.what() );
    }
    catch( ... )
    {
        LOG_ERROR( "Unknown error when updating options!" );
    }
#endif
    return updated_options;
}

}  // namespace librealsense

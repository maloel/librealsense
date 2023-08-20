// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include <rscore/device-factory-registry.h>

#include <rsutils/json.h>


namespace librealsense {


device_factories device_factory_registry::create_all( nlohmann::json const & settings )
{
    device_factories factories;
    for( auto & name_creator : the_registry() )
    {
        auto factory = ( name_creator.second )( name_creator.first );
        if( ! factory )
            continue;
        try
        {
            if( ! factory->initialize_factory( settings ) )
                continue;
            factories.push_back( factory );
        }
        catch( std::exception const & e )
        {
            LOG_ERROR( "Failed to initialize '" << name_creator.first << "' device-factory: " << e.what() );
        }
        catch( ... )
        {
            LOG_ERROR( "Failed to initialize '" << name_creator.first << "' device-factory: unknown exception" );
        }
    }
    return factories;
}


}  // namespace librealsense

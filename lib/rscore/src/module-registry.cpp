// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include <rscore/module-registry.h>
#include <rscore/context.h>

#include <rsutils/json.h>


namespace librealsense {


/*static*/ std::vector< std::shared_ptr< context_module > >
module_registry::create_context_modules( context const & ctx )
{
    std::vector< std::shared_ptr< context_module > > modules;
    for( auto & name_creator : the_registry() )
    {
        auto ctx_module = ( name_creator.second )( name_creator.first, ctx );
        if( ! ctx_module )
            continue;
        try
        {
            if( ! ctx_module->initialize_module( ctx.get_settings() ) )
                continue;
            modules.push_back( ctx_module );
        }
        catch( std::exception const & e )
        {
            LOG_ERROR( "Failed to initialize '" << name_creator.first << "' context-module: " << e.what() );
        }
        catch( ... )
        {
            LOG_ERROR( "Failed to initialize '" << name_creator.first << "' context-module: unknown exception" );
        }
    }
    return modules;
}


}  // namespace librealsense

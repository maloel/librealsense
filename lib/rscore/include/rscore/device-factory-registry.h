// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include "device-factory.h"

#include <rsutils/easylogging/easyloggingpp.h>

#include <map>
#include <memory>
#include <functional>
#include <stdexcept>


// Use this macro in a .cpp to actually add an entry - see the corresponding register_rscore_factory function in the
// makefile.
// 
// NOTE: without _RSCORE_EXPAND, the result is _RSCORE_FACTORY_NAME_registry_entry
//
#define _RSCORE_EXPAND(X) X
#define REGISTER_RSCORE_FACTORY( name )                                                                                \
    extern "C" size_t _RSCORE_EXPAND( _RSCORE_FACTORY_NAME )##_registry_entry                                          \
        = device_factory_registry::add< rs_dds_device_factory >( name )


namespace librealsense {


// Registry for all the defined device factories. These are added during initialization, based on active compiler
// modules, and then created when needed.
//
class device_factory_registry
{
    // Factory creation is delayed until a chosen time; we store a factory-factory function
    typedef std::function< std::shared_ptr< device_factory >( std::string const & /*name*/ ) > factory_fn;

    // We use a map to maintain ordering & ensure uniqueness
    typedef std::map< std::string /*name*/, factory_fn > registry_t;

    // We use a function-static variable that's initialized on first use - otherwise we cannot guarrantee that it'll be
    // initialized before add() is called:
    static registry_t & the_registry()
    {
        static registry_t _the_registry;
        return _the_registry;
    }

public:
    // To register a new factory:
    //      static auto factory_it = device_factory_registry::add< my_factory >( "my-factory" );
    //
    // The return value has no meaning; it's just there so you can assign it to something so a static variable can be
    // declared and automatically initialized by the compiler (in any order it decides on).
    //
    // The name determines ordering of the factories, and shouldn't matter. It is recommended that the name also be the
    // key in the settings passed to the factory ctor. E.g., my_factory-specific settings should ideally be placed in
    // the json '<root>/my-settings' object.
    //
    template< class T >
    static size_t add( std::string const & name )
    {
        auto & registry = the_registry();
        auto factory_was_inserted
            = registry
                  .emplace( name,
                            []( std::string const & name ) -> std::shared_ptr< device_factory >
                            {
                                std::shared_ptr< T > factory;
                                try
                                {
                                    factory = std::make_shared< T >( name );
                                }
                                catch( std::exception const & e )
                                {
                                    LOG_ERROR( "Failed to create '" << name << "' device-factory: " << e.what() );
                                }
                                catch( ... )
                                {
                                    LOG_ERROR( "Failed to create '" << name << "' device-factory: unknown exception" );
                                }
                                return factory;
                            } )
                  .second;
        if( ! factory_was_inserted )
            throw std::runtime_error( "duplicate device-factory '" + name + "' registered" );
        return registry.size();
    }

    // Call create_all() to actually instantiate the factories.
    //
    // Not all factories may be instantiated: factories may choose to remain inactive, depending on the settings or
    // other factors.
    //
    static device_factories create_all( nlohmann::json const & settings );
};


}  // namespace librealsense

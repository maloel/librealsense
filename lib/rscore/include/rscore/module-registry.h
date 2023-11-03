// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include "context-module.h"

#include <rsutils/easylogging/easyloggingpp.h>

#include <map>
#include <memory>
#include <functional>
#include <stdexcept>


// Use this macro in a .cpp to actually add a module - see the corresponding register_rscore_module function in the
// makefile:
//      REGISTER_RSCORE_MODULE( <module-class-name>, <name> );
// E.g.,
//      REGISTER_RSCORE_MODULE( d400_module, "d400" );
// See the comments below for module_resistry::add for more.
// 
// NOTE: without _RSCORE_CONCAT2, the result is _RSCORE_MODULE_NAME_registry_entry
//
#define _RSCORE_CONCAT( X, Y ) X##Y
#define _RSCORE_CONCAT2( X, Y ) _RSCORE_CONCAT( X, Y )
#define REGISTER_RSCORE_MODULE( module, name )                                                                         \
    extern "C" { size_t _RSCORE_CONCAT2( _RSCORE_MODULE_NAME, _registry_entry ) = module_registry::add< module >( name ); }


namespace librealsense {


// Registry for all the compiled module classes. Once registered, these are instantiated during context initialization
// and queried for their capabilities.
//
class module_registry
{
    // We register code, not objects: each module registers one or more module functions that get called to actually
    // create the context-specific module.
    //
    using factory_fn =  //
        std::function< std::shared_ptr< context_module >( std::string const & name, context & ) >;

    // A map to maintain ordering & ensure uniqueness
    using registry_t = std::map< std::string /*name*/, factory_fn >;

    // We use a function-static variable that's initialized on first use - otherwise we cannot guarrantee that it'll be
    // initialized before add() is called:
    static registry_t & the_registry()
    {
        static registry_t _the_registry;
        return _the_registry;
    }

public:
    // To register a new module:
    //      static auto my_module_id = module_registry::add< my_module >( "my-module" );
    // This is automatically done if you use the REGISTER_RSCORE_MODULE macro. It's better to use the macro in case the
    // implementation changes.
    //
    // The return value has no meaning; it's just there so you can assign it to a static variable that can be declared
    // and automatically initialized by the compiler (in any order it decides on). It's not meant to be used, and
    // therefore will get dropped by the linker without an additional mechanism to ensure it stays! See
    // register_rscore_module in the makefile.
    //
    // The name does determine module ordering, but shouldn't really matter except that it's unique. It is recommended
    // that the name also be the key in the settings passed to the module ctor. E.g., D400-specific settings should
    // ideally be placed in the json '<root>/d400/...' hierarchy with a "d400" module name.
    //
    template< class T >
    static size_t add( std::string const & name )
    {
        auto & registry = the_registry();
        auto module_was_inserted
            = registry
                  .emplace( name,
                            []( std::string const & name, context & ctx ) -> std::shared_ptr< context_module >
                            {
                                std::shared_ptr< T > ctx_module;
                                try
                                {
                                    ctx_module = std::make_shared< T >( name, ctx );
                                }
                                catch( std::exception const & e )
                                {
                                    LOG_ERROR( "Failed to create '" << name << "' context-module: " << e.what() );
                                }
                                catch( ... )
                                {
                                    LOG_ERROR( "Failed to create '" << name << "' context-module: unknown exception" );
                                }
                                return ctx_module;
                            } )
                  .second;
        if( ! module_was_inserted )
            throw std::runtime_error( "duplicate module '" + name + "' registered" );
        return registry.size();  // the registry never shrinks
    }

    // Call create_context_modules() to actually instantiate the modules for a given context.
    // Some modules may choose to remain inactive, depending on settings or other factors.
    //
    static std::vector< std::shared_ptr< context_module > > create_context_modules( context & );

    // If you just want to get all known modules (handy for debug):
    //
    static void foreach_registered( std::function< void( std::string const & /*name*/ ) > && );
};


}  // namespace librealsense

// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <nlohmann/json_fwd.hpp>

#include <string>
#include <memory>
#include <vector>
#include <stdexcept>


namespace librealsense {


class rscore_factory_registry;
class device_factory;
class device_info;
class context;


// Entry point into a RS module: the factory is the main object to work with a context, and from which all other
// factories are retrieved.
// 
// The derived implementation will get registered with rscore_factory_registry and, upon calling its create_all(), a
// factory will get instantiated and initialized (initialize_factory()). The factory may refuse to participate.
// 
// Each instance is for one specific context. Once initialized, a factory can be used to monitor for added/removed
// devices, maintain context-specific variables, etc.
//
class rscore_factory
{
    friend class rscore_factory_registry;

    std::string const _name;

public:
    rscore_factory( std::string const & name )
        : _name( name )
    {
        if( name.empty() )
            throw std::runtime_error( "empty name for rscore_factory" );
    }
    virtual ~rscore_factory() = default;

private:
    // This is where initialization takes place, based on settings. Called from rscore_factory_registry::create_all().
    // Exceptions are not expected.
    // Return false to not include this factory in the final set.
    //
    virtual bool initialize_factory( std::shared_ptr< context > const &, nlohmann::json const & ) = 0;

public:
    // Device monitoring
    //
    typedef std::function< void( std::shared_ptr< device_info > const & ) > device_changed_callback;

    virtual void on_device_added( device_changed_callback ) = 0;
    virtual void on_device_removed( device_changed_callback ) = 0;

    // If we can create devices, we should implement a device-factory
    //
    virtual std::shared_ptr< device_factory > get_device_factory() = 0;
};


typedef std::vector< std::shared_ptr< rscore_factory > > rscore_factories;


}  // namespace librealsense

// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <rscore/device-factory.h>

#include <nlohmann/json_fwd.hpp>
#include <string>
#include <memory>
#include <vector>
#include <stdexcept>


namespace librealsense {


class module_registry;
class context;


// The main object to interface between a module and a context, and from which all other module capabilities, in the
// form of different types of factories, are retrieved. Each instance is for one specific context.
// 
// The derived implementation will get registered with module_registry::add(). When a context, during init, calls
// create_context_modules(), we get instantiated and initialize_module() called. The module may refuse to take part
// if not applicable to the context based on settings.
//
class context_module
{
    std::string const _name;
    context & _context;

public:
    context_module( std::string const & name, context & ctx )
        : _name( name )
        , _context( ctx )
    {
        if( name.empty() )
            throw std::runtime_error( "empty context module name" );
    }

    virtual ~context_module() = default;

private:
    // This is where initialization takes place, based on context settings.
    // Called from module_registry::create_context_modules().
    // Exceptions are not expected.
    // Return false to remove this module from the context.
    //
    virtual bool initialize_module( nlohmann::json const & ) = 0;
    friend class module_registry;  // so it can call the above

public:
    // If we can create devices, we should return a device-factory; otherwise return nullptr.
    // The callback is to be called when new devices are recognized, or existing devices dropped.
    //
    virtual std::shared_ptr< device_factory > create_device_factory( device_factory::callback && ) = 0;
};


}  // namespace librealsense

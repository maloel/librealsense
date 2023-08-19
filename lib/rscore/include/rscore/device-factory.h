// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <nlohmann/json_fwd.hpp>

#include <string>
#include <memory>
#include <vector>
#include <stdexcept>


namespace librealsense {


class device_factory_registry;


class device_factory
{
    friend class device_factory_registry;

    std::string const _name;

public:
    device_factory( std::string const & name )
        : _name( name )
    {
        if( name.empty() )
            throw std::runtime_error( "empty name in device_factory" );
    }
    virtual ~device_factory() {}

private:
    // This is where initialization takes place, based on settings. Called from device_factory_registry::create_all().
    // Exceptions are not expected.
    // Return false to not include this factory in the final set.
    //
    virtual bool initialize_factory( nlohmann::json const & ) = 0;
};


typedef std::vector< std::shared_ptr< device_factory > > device_factories;


}  // namespace librealsense

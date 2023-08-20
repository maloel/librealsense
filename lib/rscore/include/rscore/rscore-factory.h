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
    virtual ~rscore_factory() {}

private:
    // This is where initialization takes place, based on settings. Called from rscore_factory_registry::create_all().
    // Exceptions are not expected.
    // Return false to not include this factory in the final set.
    //
    virtual bool initialize_factory( nlohmann::json const & ) = 0;
};


typedef std::vector< std::shared_ptr< rscore_factory > > rscore_factories;


}  // namespace librealsense

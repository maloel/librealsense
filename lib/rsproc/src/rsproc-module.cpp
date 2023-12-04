// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include <rscore/module-registry.h>
#include "rsproc-pp-block-factory.h"

#include <rsutils/easylogging/easyloggingpp.h>
#include <rsutils/json.h>


namespace librealsense {


class rsproc_module : public context_module
{
    typedef context_module super;

    nlohmann::json _settings;

public:
    rsproc_module( std::string const & name, context & ctx )
        : super( name, ctx )
    {
    }

    bool is_enabled() const { return _settings.is_object(); }

public:
    bool initialize_module( nlohmann::json const & settings ) override
    {
        // Missing "proc" in settings: enabled; use defaults
        // If "proc" is not an object: disabled
        if( ! rsutils::json::get_ex( settings, _name, &_settings ) )
            _settings = nlohmann::json::object();
        return is_enabled();
    }

    std::shared_ptr< device_factory > create_device_factory( device_factory::callback && cb ) override
    {
        return {};  // we don't create devices
    }

    std::shared_ptr< pp_block_factory > create_pp_block_factory() override
    {
        if( ! is_enabled() )
            throw std::runtime_error( _name + " module is disabled" );

        return std::make_shared< rsproc_pp_block_factory >( _settings );
    }
};


REGISTER_RSCORE_MODULE( rsproc_module, "proc" )


}  // namespace librealsense
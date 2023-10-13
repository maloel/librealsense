// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include <rscore/module-registry.h>
#include <src/dds/rsdds-device-factory.h>

#include <rsutils/easylogging/easyloggingpp.h>
#include <rsutils/json.h>


namespace librealsense {


class rsdds_module : public context_module
{
    typedef context_module super;

    nlohmann::json _settings;

public:
    rsdds_module( std::string const & name, context & ctx )
        : super( name, ctx )
    {
    }

    bool is_enabled() const { return _settings.is_object(); }

public:
    bool initialize_module( nlohmann::json const & settings ) override
    {
        // Missing "dds" in settings: enabled; use defaults
        // If "dds" is not an object: disabled
        if( ! rsutils::json::get_ex( settings, _name, &_settings ) )
            _settings = nlohmann::json::object();
        return is_enabled();
    }

    std::shared_ptr< device_factory > create_device_factory( device_factory::callback && cb ) override
    {
        if( ! is_enabled() )
            throw std::runtime_error( _name + " module is disabled" );
        return std::make_shared< rsdds_device_factory >( _context, std::move( cb ) );
    }
};


REGISTER_RSCORE_MODULE( rsdds_module, "dds" )


}  // namespace librealsense
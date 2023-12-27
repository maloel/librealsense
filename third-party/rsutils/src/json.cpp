// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include <rsutils/json.h>
#include <rsutils/os/executable-name.h>

namespace rsutils {


json_type const null_json = {};
json_type const empty_json_string = json_type::value_type( json_type::value_t::string );
json_type const empty_json_object = json_type::object();


/*static*/ void json::patch( json_type & j, json_type const & patches, std::string const & what )
{
    if( ! patches.is_object() )
    {
        std::string context = what.empty() ? std::string( "patch", 5 ) : what;
        throw std::runtime_error( context + ": expecting an object; got " + patches.dump() );
    }

    try
    {
        j.merge_patch( patches );
    }
    catch( std::exception const & e )
    {
        std::string context = what.empty() ? std::string( "patch", 5 ) : what;
        throw std::runtime_error( "failed to merge " + context + ": " + e.what() );
    }
}


/*static*/ json_type json::load_app_settings( json_type const & global,
                                              std::string const & application,
                                              json_key const & subkey,
                                              std::string const & error_context )
{
    // Take the global subkey settings out of the configuration
    nlohmann::json settings;
    if( auto global_subkey = json_ref( global, subkey ) )
        patch( settings, global_subkey, "global " + error_context + '/' + subkey );

    // Patch any application-specific subkey settings
    if( auto application_subkey = json_ref( global, application, subkey ) )
        patch( settings, application_subkey, error_context + '/' + application + '/' + subkey );

    return settings;
}


/*static*/ json_type
json::load_settings( json_type const & global, json_key const & subkey, std::string const & error_context )
{
    return load_app_settings( global, rsutils::os::executable_name(), subkey, error_context );
}


}  // namespace rsutils

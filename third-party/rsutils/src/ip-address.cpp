// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#include <rsutils/string/ip-address.h>

#include <rsutils/json.h>
using rsutils::json;


namespace rsutils {
namespace string {


ip_address::ip_address( std::string str )
{
    char const * pch = str.c_str();
    char const * const end = pch + str.length();
    while( *pch >= '0' && *pch <= '9' )
        ++pch;
    if( *pch++ != '.' )
        return;
    while( *pch >= '0' && *pch <= '9' )
        ++pch;
    if( *pch++ != '.' )
        return;
    while( *pch >= '0' && *pch <= '9' )
        ++pch;
    if( *pch++ != '.' )
        return;
    while( *pch >= '0' && *pch <= '9' )
        ++pch;
    if( *pch )
        return;
    _str = std::move( str );
}


void to_json( json & j, const ip_address & ip )
{
    j = ip.to_string();
}


void from_json( json const & j, ip_address & ip )
{
    if( ! j.is_string() )
        throw rsutils::json::type_error::create( 317, "ip_address should be a string", &j );

    ip_address tmp( j.string_ref() );
    if( ! ip.is_valid() )
        throw rsutils::json::type_error::create( 317, "invalid ip_address", &j );

    ip = std::move( tmp );
}


}  // namespace string
}  // namespace rsutils
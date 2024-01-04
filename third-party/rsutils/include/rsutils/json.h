// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.
#pragma once

#include "json-fwd.h"
#include <nlohmann/json.hpp>


namespace rsutils {


template< typename... Rest >
json_ref _nested_json( json const & j );
template< typename... Rest >
json_ref _nested_json( json const & j, bool (json:: * is_fn)() const );
template< typename... Rest >
json_ref _nested_json( json const & j, json_key const & inner, Rest... rest );
template< typename... Rest >
json_ref _nested_json( json const & j, json::size_type index, Rest... rest );


#if 0
inline std::string _nested_json_path( json_key const & a ) { return a; }
inline std::string _nested_json_path( json::size_type x ) { return '[' + std::to_string( x ) + ']'; }

template< typename... Rest >
std::string _nested_json_path( json_key const & a, json_key const & b, Rest... rest )
{
    return a + '/' + _nested_json_path( b, std::forward< Rest >( rest )... );
}
template< typename... Rest >
std::string _nested_json_path( json_key const & a, json::size_type b, Rest... rest )
{
    return a + _nested_json_path( b, std::forward< Rest >( rest )... );
}
template< typename... Rest >
std::string _nested_json_path( json::size_type a, json::size_type b, Rest... rest )
{
    return _nested_json_path( a ) + _nested_json_path( b, std::forward< Rest >( rest )... );
}
template< typename... Rest >
std::string _nested_json_path( json::size_type a, json_key const & b, Rest... rest )
{
    return _nested_json_path( a ) + '/' + _nested_json_path( b, std::forward< Rest >( rest )... );
}
template< typename... Rest >
static std::string _nested_json_path( json::size_type index, Rest... rest )
{
    return a + '[' + std::to_string( index ) + ']' + _nested_json_path( std::forward< Rest >( rest )... );
}
#endif


// Allow easy read-only lookup of nested json hierarchies:
//      j["one"]["two"]["three"]            // undefined; will throw/assert if a hierarchy isn't there
// But:
//      nested( j, "one", "two", "three" )  // will not throw; does not copy!
// The result is either a null JSON object or a valid one. The boolean operator can be used as an easy check:
//      if( auto inside = nested( j, "one", "two" ) )
//          { ... }
//
class json_ref
{
    json const & _j;

public:
    json_ref() : _j( missing_json ) {}
    json_ref( json const & j ) : _j( j ) {}

    template< typename... Rest >
    json_ref( json const & j, Rest... rest )
        : _j( _nested_json( j, std::forward< Rest >( rest )... ) )
    {}

    constexpr bool exists() const noexcept { return ! _j.is_discarded(); }
    operator bool() const noexcept { return exists(); }

    constexpr json const & get_json() const noexcept { return _j; }
    operator json const &() const noexcept { return get_json(); }

    constexpr bool is_null() const noexcept { return _j.is_null(); }
    constexpr bool is_array() const noexcept { return _j.is_array(); }
    constexpr bool is_object() const noexcept { return _j.is_object(); }
    constexpr bool is_string() const noexcept { return _j.is_string(); }
    constexpr bool is_boolean() const noexcept { return _j.is_boolean(); }
    constexpr bool is_number() const noexcept { return _j.is_number(); }
    constexpr bool is_number_float() const noexcept { return _j.is_number_float(); }
    constexpr bool is_number_integer() const noexcept { return _j.is_number_integer(); }
    constexpr bool is_number_unsigned() const noexcept { return _j.is_number_unsigned(); }
    constexpr bool is_primitive() const noexcept { return _j.is_primitive(); }
    constexpr bool is_structured() const noexcept { return _j.is_structured(); }

    bool empty() const { return _j.empty(); }
    json::size_type size() const { return _j.size(); }
    json::const_iterator begin() const { return _j.begin(); }
    json::const_iterator end() const { return _j.end(); }

    std::string dump( const int indent = -1 ) const { return _j.dump( indent ); }

    // Dig deeper
    template< typename... Rest >
    inline json_ref nested( Rest... rest ) const
    {
        return _nested_json( _j, std::forward< Rest >( rest )... );
    }

#if 0
    // Same, but throws
    template< typename... Rest >
    inline json_ref nested_check( Rest... rest ) const
    {
        if( auto jr = _nested_json( _j, std::forward< Rest >( rest )... ) )
            return jr;
        throw std::runtime_error( "key not found: " + _nested_json_path( std::forward< Rest >( rest )... ) );
    }
#endif

    template< class Key > inline json::const_reference at( Key key ) const { return _j.at( std::forward< Key >( key ) ); }
    template< class Key > inline json::const_reference operator[]( Key key ) const { return _j.operator[]( key ); }
    template< class T > inline T get() const { return _j.get< T >(); }
    template< class T > inline void get_to( T & value ) const { _j.get_to( value ); }

    // If there, gets the value at the given key and returns true; otherwise false
    template< class T >
    bool get_ex( T & value ) const
    {
        if( ! exists() )
            return false;
        _j.get_to( value );
        return true;
    }

    // Get the JSON as a value, or a default if not there (throws if wrong type)
    template< class T >
    constexpr T default_value( T const & default_value ) const noexcept
    {
        return exists() ? _j.get< T >() : default_value;
    }

    // Get the object, with a default being an empty one; does not throw
    inline constexpr json const & default_object() const noexcept { return is_object() ? _j : empty_json_object; }

    // Get the string object, with a default being an empty one; does not throw
    inline constexpr json const & default_string() const noexcept { return is_string() ? _j : empty_json_string; }

    // Get a JSON string by reference (zero copy); it must be a string or it'll throw
    inline std::string const & string_ref() const { return _j.get_ref< const json::string_t & >(); }

    // Get a JSON string by reference (zero copy); does not throw
    inline std::string const & string_ref_or_empty() const { return default_string().get_ref< const json::string_t & >(); }
};


inline std::ostream & operator<<( std::ostream & os, json_ref const & j )
{
    return operator<<( os, static_cast< json const & >( j ) );
}


template< typename... Rest >
json_ref _nested_json( json const & j )
{
    // j.nested()
    return j;
}
template< typename... Rest >
json_ref _nested_json( json const & j, bool ( json::*is_fn )() const )
{
    // j.nested( &json::is_string )
    if( ! ( j.*is_fn )() )
        return missing_json;
    return j;
}
template< typename... Rest >
json_ref _nested_json( json const & j, json_key const & inner, Rest... rest )
{
    // j.nested( "key", ... )
    auto it = j.find( inner );
    if( it == j.end() )
        return missing_json;
    return _nested_json( *it, std::forward< Rest >( rest )... );
}
template< typename... Rest >
json_ref _nested_json( json const & j, json::size_type index, Rest... rest )
{
    // j.nested( index, ... )
    if( ! j.is_array() )
        return missing_json;
    if( index >= j.size() )
        return missing_json;
    return _nested_json( j[index], std::forward< Rest >( rest )... );
}


// Since we know how we're derived from, we can get the json, or the reference to it, from this:
//     json <- nlohmann::basic_json<...> <- json_base
inline json_ref json_base::_ref() const
{
    return static_cast< json const & >( *this );
}


// Returns false if the object wasn't found
inline bool json_base::exists() const
{
    return _ref().exists();
}

// Get the JSON as a value, or a default if not there (throws if wrong type)
template< class T >
inline T json_base::default_value( T const & default_value ) const
{
    return _ref().default_value( default_value );
}

// If there, gets the value at the given key and returns true; otherwise false
template< class T >
inline bool json_base::get_ex( T & value ) const
{
    return _ref().get_ex( value );
}

// Get the object, with a default being an empty one; does not throw
inline json_ref json_base::default_object() const
{
    return _ref().default_object();
}

// Get the string object, with a default being an empty one; does not throw
inline json_ref json_base::default_string() const
{
    return _ref().default_string();
}

// Get a JSON string by reference (zero copy); it must be a string or it'll throw
inline std::string const & json_base::string_ref() const
{
    return _ref().string_ref();
}

// Get a JSON string by reference (zero copy); does not throw
inline std::string const & json_base::string_ref_or_empty() const
{
    return default_string().string_ref();
}

// Allow easy read-only lookup of nested json hierarchies:
//      j["one"]["two"]["three"]            // undefined; will throw/assert if a hierarchy isn't there
// But:
//      nested( j, "one", "two", "three" )  // will not throw; does not copy!
// The result is either a null JSON object or a valid one. The boolean operator can be used as an easy check:
//      if( auto inside = nested( j, "one", "two" ) )
//          { ... }
//
template< typename... Rest >
inline json_ref json_base::nested( Rest... rest ) const
{
    return _ref().nested( std::forward< Rest >( rest )... );
}

#if 0
// Same, but throws
template< typename... Rest >
inline json_ref json_base::nested_check( Rest... rest ) const
{
    return _ref().nested_check( std::forward< Rest >( rest )... );
}
#endif


}  // namespace rsutils

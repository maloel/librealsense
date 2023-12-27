// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.
#pragma once

#include <nlohmann/json.hpp>
#include <string>


namespace rsutils {


using json_key = std::string;
using json_type = nlohmann::json;

class json_ref;  // forward decl


extern json_type const null_json;
extern json_type const empty_json_string;
extern json_type const empty_json_object;


class json : public json_type
{
public:


    // Returns true if the json has a certain key.
    // Does not check the value at all, so it could be any type or null.
    static bool has( json_type const & j, json_key const & key )
    {
        auto it = j.find( key );
        if( it == j.end() )
            return false;
        return true;
    }


    // Returns true if the json has a certain key and its value is not null.
    // Does not check the value type.
    static bool has_value( json_type const & j, json_key const & key )
    {
        auto it = j.find( key );
        if( it == j.end() || it->is_null() )
            return false;
        return true;
    }


    // Get the JSON as a value (copy involved); it must exist
    template < class T >
    static T value( json_type const & j )
    {
        return j.get< T >();
    }
    // Get the JSON as a value, or a default if not there (copy involved)
    template < class T >
    static T value( json_type const & j, T const & default_value )
    {
        if( j.is_null() )
            return default_value;
        return j.get< T >();
    }
    // Get a JSON string by reference (zero copy); it must be a string or it'll throw
    static std::string const & string_ref( json_type const & j )
    {
        return j.get_ref< const json_type::string_t & >();
    }


    // If there, gets the value at the given key and returns true; otherwise false.
    // Turns json exceptions into runtime errors with additional info.
    template< class T >
    static bool get_ex( json_type const & j, json_key const & key, T * pv )
    {
        auto it = j.find( key );
        if( it == j.end() || it->is_null() )
            return false;
        try
        {
            // This will throw for type mismatches, etc.
            it->get_to( *pv );
        }
        catch( json_type::exception & e )
        {
            throw std::runtime_error( "[while getting '" + key + "']" + e.what() );
        }
        return true;
    }


    // If there, returns the value at the given key; otherwise returns a default value.
    template< class T >
    static T get( json_type const & j, json_key const & key, T const & default_value )
    {
        if( ! j.is_object() )
            return default_value;
        return j.value( key, default_value );
    }


    // If there, returns the value at the given key; otherwise throws!
    // Turns json exceptions into runtime errors with additional info.
    template< class T >
    static T get( json_type const & j, json_key const & key )
    {
        // This will throw for type mismatches, etc.
        // Does not check for existence: will throw, too!
        return j.at(key).get< T >();
    }


    // If there, returns the value at the given index (in an array); otherwise throws!
    // Turns json exceptions into runtime errors with additional info.
    template< class T >
    static T get( json_type const & j, int index )
    {
        // This will throw for type mismatches, etc.
        // Does not check for existence: will throw, too!
        return j.at( index ).get< T >();
    }


    // If there, returns the value at the given iterator; otherwise throws!
    // Turns json exceptions into runtime errors with additional info.
    template < class T >
    static T get( json_type const & j, json_type::const_iterator const & it )
    {
        if( it == j.end() )
            throw std::runtime_error( "unexpected end of json" );
        // This will throw for type mismatches, etc.
        // Does not check for existence: will throw, too!
        return it->get< T >();
    }


    template< typename... Rest >
    static json_ref nested( json_type const & j )
    {
        return j;
    }
    template< typename... Rest >
    static json_ref nested( json_type const & j, json_key const & inner, Rest... rest )
    {
        auto it = j.find( inner );
        if( it == j.end() )
            return null_json;
        return nested( *it, std::forward< Rest >( rest )... );
    }


    // Recursively patches existing 'j' with contents of 'patches', which must be a JSON object.
    // A 'null' value inside erases previous contents. Any other value overrides.
    // See: https://json.nlohmann.me/api/basic_json/merge_patch/
    // Example below, for load_app_settings.
    // Use 'what' to denote what it is we're patching in, if a failure happens. The std::runtime_error will populate with
    // it.
    //
    static void patch( json_type & j, json_type const & patches, std::string const & what = {} );


    // Loads configuration settings from 'global' content.
    // E.g., a configuration file may contain:
    //     {
    //         "context": {
    //             "dds": {
    //                 "enabled": false,
    //                 "domain" : 5
    //             }
    //         },
    //         ...
    //     }
    // This function will load a specific key 'context' inside and return it. The result will be a disabling of dds:
    // Besides this "global" key, application-specific settings can override the global settings, e.g.:
    //     {
    //         "context": {
    //             "dds": {
    //                 "enabled": false,
    //                 "domain" : 5
    //             }
    //         },
    //         "realsense-viewer": {
    //             "context": {
    //                 "dds": { "enabled": null }
    //             }
    //         },
    //         ...
    //     }
    // If the current application is 'realsense-viewer', then the global 'context' settings will be patched with the
    // application-specific 'context' and returned:
    //     {
    //         "dds": {
    //             "domain" : 5
    //         }
    //     }
    // See rules for patching in patch().
    // The 'application' is usually any single-word executable name (without extension).
    // The 'subkey' is mandatory.
    // The 'error_context' is used for error reporting, to show what failed. Like application, it should be a single word
    // that can be used to denote hierarchy within the global json.
    //
    static json_type load_app_settings( json_type const & global,
                                        std::string const & application,
                                        json_key const & subkey,
                                        std::string const & error_context );


    // Same as above, but automatically takes the application name from the executable-name.
    //
    static json_type load_settings( json_type const & global,
                                    json_key const & subkey,
                                    std::string const & error_context );


};


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
    json_type const & _j;

public:
    json_ref() : _j( null_json ) {}
    json_ref( json_type const & j ) : _j( j ) {}

    template< typename... Rest >
    json_ref( json_type const & j, Rest... rest )
        : _j( json::nested( j, std::forward< Rest >( rest )... ) )
    {}

    json_type const * operator->() const { return &_j; }

    bool exists() const { return !_j.is_null(); }
    operator bool() const { return exists(); }

    json_type const & get() const { return _j; }
    operator json_type const & () const { return get(); }

    bool is_array() const { return _j.is_array(); }
    bool is_object() const { return _j.is_object(); }
    bool is_string() const { return _j.is_string(); }

    // Dig deeper
    template< typename... Rest >
    inline json_ref find( Rest... rest ) const
    {
        return json::nested( _j, std::forward< Rest >( rest )... );
    }
    inline json_ref operator[]( json_key const & key ) const { return find( key ); }

    // Get the JSON as a value
    template< class T > T value() const { return json::value< T >( get() ); }
    // Get the JSON as a value, or a default if not there (throws if wrong type)
    template < class T > T default_value( T const & default_value ) const { return json::value< T >( get(), default_value ); }
    // Get the object, with a default being an empty one; does not throw
    json_type const & default_object() const { return is_object() ? _j : empty_json_object; }
    // Get the object, with a default being an empty one; does not throw
    json_type const & default_string() const { return is_string() ? _j : empty_json_string; }
    // Get a JSON string by reference (zero copy); it must be a string or it'll throw
    inline std::string const & string_ref() const { return json::string_ref( get() ); }
    // Get a JSON string by reference (zero copy); does not throw
    inline std::string const & string_ref_or_empty() const { return json::string_ref( default_string() ); }
};


}  // namespace rsutils

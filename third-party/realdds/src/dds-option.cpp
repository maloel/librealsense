// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/dds-option.h>
#include <realdds/dds-exceptions.h>

#include <rsutils/json.h>
using rsutils::json;


namespace realdds {


dds_option::dds_option( const std::string & name,
                        rsutils::json const & value,
                        rsutils::json const & default_value,
                        std::string const & description,
                        option_properties const & props )
    : _name( name )
    , _j( value )
    , _default_j( default_value )
    , _description( description )
{
    if( default_value.type() != value.type() )
        DDS_THROW( runtime_error,
                   "mismatch between value (" << value << ") and default value (" << default_value << ") type" );
    if( ! props.empty() )
    {
        DDS_THROW( runtime_error, "invalid option properties" );
    }
}


//dds_option::dds_option( rsutils::json const & j )
//{
//    int index = 0;
//
//    _name                = j[index++].string_ref();
//    _value               = j[index++].get< float >();
//    _range.min           = j[index++].get< float >();
//    _range.max           = j[index++].get< float >();
//    _range.step          = j[index++].get< float >();
//    _range.default_value = j[index++].get< float >();
//    _description         = j[index++].string_ref();
//
//    if( index != j.size() )
//        DDS_THROW( runtime_error, "expected end of json at index " << index );
//}


void dds_option::init_stream( std::shared_ptr< dds_stream_base > const & stream )
{
    if( _stream.lock() )
        DDS_THROW( runtime_error, "option '" << get_name() << "' already has a stream" );
    if( ! stream )
        DDS_THROW( runtime_error, "null stream" );
    _stream = stream;
}


static dds_option_type option_type_from_json_value( json const & j )
{
    switch( j.type() )
    {
    //case json::value_t::null:
    //case json::value_t::object:
    //case json::value_t::array:
    case json::value_t::number_float:
        return dds_option_type::float_type;
    case json::value_t::string:
        return dds_option_type::string_type;
    case json::value_t::boolean:
    case json::value_t::number_integer:
    case json::value_t::number_unsigned:
        return dds_option_type::number_type;
    //case json::value_t::binary:
    //case json::value_t::discarded:
    default:
        return dds_option_type::unknown_type;
    }
}


static dds_option::option_properties parse_option_properties( json const & j, size_t index )
{
    dds_option::option_properties props;
    for( ; index < j.size(); ++index )
    {
        auto & jx = j[index];
        if( ! jx.is_string() )
            DDS_THROW( runtime_error, "invalid option property " << index << " type in: " << j );

        auto & prop = jx.string_ref();
        if( ! props.insert( prop ).second )
            DDS_THROW( runtime_error, "option property '" << prop << "' repeats: " << j );
    }
    return props;
}


/*static*/ std::shared_ptr< dds_option > dds_option::from_json( rsutils::json const & j )
{
    if( ! j.is_array() )
        DDS_THROW( runtime_error, "option expected as array: " << j );
    auto const size = j.size();

    if( size < 1 )
        DDS_THROW( runtime_error, "no option name: " << j );
    auto & name = j[0];
    if( name.string_ref_or_empty().empty() )
        DDS_THROW( runtime_error, "invalid option name: " << j );

    if( size < 2 )
        DDS_THROW( runtime_error, "no option value: " << j );
    auto & value = j[1];
    switch( value.type() )
    {
    case json::value_t::number_float: {
        bool const is_valid = size >= 7          // [name,value,min,max,step,default,description]
                           && j[2].is_number()   // min
                           && j[3].is_number()   // max
                           && j[4].is_number()   // step
                           && j[5].is_number()   // default
                           && j[6].is_string();  // description
        if( ! is_valid )
            DDS_THROW( runtime_error, "invalid float option: " << j );
        return std::make_shared< dds_float_option >( name.string_ref(),
                                                     value.get< float >(),
                                                     dds_float_option::range{ j[2].get< float >(),
                                                                              j[3].get< float >(),
                                                                              j[4].get< float >() },
                                                     j[5].get< float >(),
                                                     j[6].string_ref(),
                                                     parse_option_properties( j, 7 ) );
    }
    //case json::value_t::boolean: {
    //    bool const is_valid = size >= 4          // [name,value,default,description]
    //                       && j[2].is_boolean()  // default
    //                       && j[3].is_string();  // description
    //    if( ! is_valid )
    //        DDS_THROW( runtime_error, "invalid boolean option: " << j );
    //    //return std::make_shared< dds_bool_option >( name.string_ref(),
    //    //                                             dds_float_option::range{
    //    //                                                 j[2].get< float >(),
    //    //                                                 j[3].get< float >(),
    //    //                                                 j[4].get< float >(),
    //    //                                                 j[5].get< float >()
    //    //                                             },
    //    //                                             j[6].string_ref() );
    //    break;
    default: {
        size_t x_props = 4;  // [name,value,default,description]
        bool const have_range = size >= 5                    // [name,value,range,default,description]
                             && j[2].type() != value.type()  // range would be something else, like an array
                             && j[3].is_string()             // default
                             && j[4].is_string();            // description
        if( have_range )
            ++x_props;
        auto props = parse_option_properties( j, x_props );
        if( props.erase( "IPv4" ) )
        {
            if( have_range )
                DDS_THROW( runtime_error, "IPv4 option cannot have a range: " << j );
            return std::make_shared< dds_ip_option >( name.string_ref(), value, j[3], j[4], props );
        }
        // We don't know of any other types; assume a string type
        if( ! value.is_string() )
            DDS_THROW( runtime_error, "unknown option value type: " << j );
        if( have_range )
            DDS_THROW( runtime_error, "string type does not support a range: " << j );
        return std::make_shared< dds_string_option >( name.string_ref(), value, j[3], j[4], props );
    }
    }
}


rsutils::json dds_option::to_json() const
{
    return rsutils::json::array( { _name, _j, _default_j, _description } );
}


dds_float_option::dds_float_option( const std::string & name,
                                    float value,
                                    range const & range,
                                    float default_value,
                                    std::string const & description,
                                    option_properties const & properties )
    : super( name, value, default_value, description, properties )
    , _range( range )
{
}


/*static*/ bool dds_float_option::check_json( json const & value )
{
    return value.is_number_float();
}


void dds_float_option::check_value( json const & value )
{
    if( ! check_json( value ) )
        DDS_THROW( runtime_error, "not a float: " << value );
}


rsutils::json dds_float_option::to_json() const
{
    return rsutils::json::array( { _name, _j, _range.min, _range.max, _range.step, _default_j, _description } );
}


dds_string_option::dds_string_option( const std::string & name,
                                      std::string const & value,
                                      std::string const & default_value,
                                      std::string const & description,
                                      option_properties const & properties )
    : super( name, value, default_value, description, properties )
{
}


/*static*/ bool dds_string_option::check_json( json const & value )
{
    return value.is_string();
}


void dds_string_option::check_value( json const & value )
{
    if( ! check_json( value ) )
        DDS_THROW( runtime_error, "not a string: " << value );
}


dds_ip_option::dds_ip_option( const std::string & name,
                              ip_address const & value,
                              ip_address const & default_value,
                              std::string const & description,
                              option_properties const & properties )
    : super( name, json( value ), json( default_value ), description, properties )
{
}


/*static*/ bool dds_ip_option::check_json( json const & value )
{
    if( ! super::check_json( value ) )
        return false;

    return ip_address( value.string_ref() ).is_valid();
}


void dds_ip_option::check_value( json const & value )
{
    if( ! check_json( value ) )
        DDS_THROW( runtime_error, "not an IP address: " << value );
}


}  // namespace realdds

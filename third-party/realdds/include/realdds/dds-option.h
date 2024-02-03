// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.
#pragma once

#include <rsutils/json.h>
#include <rsutils/string/ip-address.h>

#include <string>
#include <vector>
#include <memory>
#include <set>


namespace realdds {


struct dds_option_range
{
    float min = 0.0f;
    float max = 0.0f;
    float step = 0.0f;
    float default_value = 0.0f;
};

enum class dds_option_type
{
    unknown_type,
    number_type,
    float_type,
    string_type,
    ip_type,
};

class dds_stream_base;

class dds_option
{
protected:
    std::string _name;
    rsutils::json _j;
    rsutils::json _default_j;
    std::string _description;

private:
    friend class dds_stream_base;
    std::weak_ptr< dds_stream_base > _stream;
    void init_stream( std::shared_ptr< dds_stream_base > const & );

public:
    using option_properties = std::set< std::string >;

protected:
    dds_option( const std::string & name,
                rsutils::json const & value,
                rsutils::json const & default_value,
                std::string const & description,
                option_properties const & );

public:
    const std::string & get_name() const { return _name; }
    std::shared_ptr< dds_stream_base > stream() const { return _stream.lock(); }

    rsutils::json const & get_value() const { return _j; }
    void set_value( rsutils::json const & value )
    {
        check_value( value );
        _j = value;
    }
    virtual void check_value( rsutils::json const & value ) = 0;

    const std::string & get_description() const { return _description; }

    virtual rsutils::json to_json() const;
    static std::shared_ptr< dds_option > from_json( rsutils::json const & j );
};

typedef std::vector< std::shared_ptr< dds_option > > dds_options;


class dds_float_option : public dds_option
{
    using super = dds_option;

public:
    struct range
    {
        float min = 0.0f;
        float max = 0.0f;
        float step = 0.0f;
    };

private:
    range const _range;

public:
    dds_float_option( const std::string & name,
                      float value,
                      range const &,
                      float default_value,
                      std::string const & description,
                      option_properties const & );

    float get_float() const { return get_value().get< float >(); }
    range const & get_range() const { return _range; }

    void check_value( rsutils::json const & ) override;
    static bool check_json( rsutils::json const & );

    rsutils::json to_json() const override;
};


class dds_string_option : public dds_option
{
    using super = dds_option;

public:
    dds_string_option( const std::string & name,
                       std::string const & value,
                       std::string const & default_value,
                       std::string const & description,
                       option_properties const & );

    void check_value( rsutils::json const & ) override;
    static bool check_json( rsutils::json const & );

    std::string get_string() const { return get_value().get< std::string >(); }
};


class dds_ip_option : public dds_string_option
{
    using super = dds_string_option;

public:
    using ip_address = rsutils::string::ip_address;

    dds_ip_option( const std::string & name,
                   ip_address const & value,
                   ip_address const & default_value,
                   std::string const & description,
                   option_properties const & );

    void check_value( rsutils::json const & ) override;
    static bool check_json( rsutils::json const & );

    ip_address get_ip() const { return ip_address( get_value().string_ref() ); }
};



}  // namespace realdds

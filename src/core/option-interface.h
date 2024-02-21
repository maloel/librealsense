// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.
#pragma once

#include "extension.h"
#include <src/basics.h>

enum rs2_option_type;


namespace librealsense {


struct LRS_EXTENSION_API option_range
{
    float min;
    float max;
    float step;
    float def;
};

class LRS_EXTENSION_API option : public recordable< option >
{
public:
    virtual void set( float value ) = 0;
    virtual float query() const = 0;

    // Return the type of the option value
    virtual rs2_option_type get_value_type() const noexcept;

    virtual option_range get_range() const = 0;
    virtual bool is_enabled() const = 0;
    virtual bool is_read_only() const { return false; }
    virtual const char * get_description() const = 0;
    virtual const char * get_value_description( float ) const { return nullptr; }
    
    // recordable< option >
    virtual void create_snapshot( std::shared_ptr< option > & snapshot ) const override;

    virtual ~option() = default;
};


}  // namespace librealsense

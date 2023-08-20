// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <rscore/device-factory.h>


namespace librealsense {


class rs_dds_device_factory : public device_factory
{
    typedef device_factory super;

public:
    rs_dds_device_factory( std::string const & name );

    bool initialize_factory( nlohmann::json const & settings ) override;
};


}  // namespace librealsense
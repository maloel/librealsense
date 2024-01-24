// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.
#pragma once

#include <src/core/options-watcher.h>


namespace realdds {
class dds_device;
}


namespace librealsense {


// 
class dds_options_watcher : public options_watcher
{
    std::shared_ptr< realdds::dds_device > const _dds_dev;
    std::string const _sensor_name;

public:
    dds_options_watcher( std::shared_ptr< realdds::dds_device > const & device, std::string const & sensor_name );

protected:
    options_and_values update_options() override;


};


}  // namespace librealsense

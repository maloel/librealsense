// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include <rsdds/rs-dds-device-factory.h>
#include <rscore/device-factory-registry.h>


using namespace librealsense;


extern "C" {
auto rs_dds_registry_entry = device_factory_registry::add< rs_dds_device_factory >( "dds" );
}

namespace {

struct registry_entry
{
    registry_entry()
    {
        //device_factory_registry::add< rs_dds_device_factory >( "dds" );
        (void *) &rs_dds_registry_entry;
    }
} rs_dds_registry_entry2;


auto bar = &rs_dds_registry_entry;

}



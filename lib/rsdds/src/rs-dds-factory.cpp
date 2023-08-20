// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include <rsutils/easylogging/easyloggingpp.h>

#if 0
#include "dds/rs-dds-device-info.h"

#include <realdds/dds-device-watcher.h>
#include <realdds/dds-participant.h>
#include <realdds/dds-device.h>
#include <realdds/topics/device-info-msg.h>
#include <rsutils/shared-ptr-singleton.h>
#include <rsutils/os/executable-name.h>
#endif

#include <rscore/factory-registry.h>
#include <rsutils/json.h>


namespace librealsense {


class rs_dds_factory : public rscore_factory
{
    typedef rscore_factory super;

public:
    rs_dds_factory( std::string const & name )
        : super( name )
    {
        LOG_DEBUG( "rs-dds-factory( " << name << " )" );
    }

    bool initialize_factory( nlohmann::json const & settings ) override
    {
        LOG_DEBUG( "rs-dds-factory::initialize_factory( " << settings << " )" );
        return true;
    }
};


#if 0
// We manage one participant and device-watcher per domain:
// Two contexts with the same domain-id will share the same participant and watcher, while a third context on a
// different domain will have its own.
//
struct dds_domain_context
{
    rsutils::shared_ptr_singleton< realdds::dds_participant > participant;
    rsutils::shared_ptr_singleton< realdds::dds_device_watcher > device_watcher;
};
//
// Domains are mapped by ID:
// Two contexts with the same participant name on different domain-ids are using two different participants!
//
static std::map< realdds::dds_domain_id, dds_domain_context > dds_domain_context_by_id;
#endif


REGISTER_RSCORE_FACTORY( rs_dds_factory, "dds" );


}  // namespace librealsense
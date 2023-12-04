// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.
#pragma once

#include <rscore/pp-block-factory.h>
#include <nlohmann/json.hpp>


namespace librealsense {


class rsproc_pp_block_factory : public pp_block_factory
{
    nlohmann::json _settings;

public:
    rsproc_pp_block_factory( nlohmann::json const & settings )
        : _settings( settings )
    {
    }

    // pp_block_factory
public:
    std::shared_ptr< processing_block_interface > create_pp_block( std::string const & name,
                                                                   nlohmann::json const & settings ) override;
};


}  // namespace librealsense

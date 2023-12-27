// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.
#pragma once

#include <nlohmann/json_fwd.hpp>


namespace rsutils {


using json_key = std::string;
using json_type = nlohmann::json;

class json_ref;
class json;


extern json_type const null_json;
extern json_type const empty_json_string;
extern json_type const empty_json_object;


}  // namespace rsutils

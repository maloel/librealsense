// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include <rscore/exceptions.h>


namespace librealsense {


recoverable_exception::recoverable_exception( const std::string & msg, rs2_exception_type exception_type ) noexcept
    : librealsense_exception( msg, exception_type )
{
    LOG_DEBUG( "recoverable_exception: " << msg );
}


}  // namespace librealsense

// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.
#pragma once

#include <librealsense2/hpp/rs_types.hpp>
#include <memory>


namespace librealsense {


class frame_interface;


template< class T >
class frame_callback : public rs2_frame_callback
{
    T on_frame_function;  // Callable of type: void(frame_interface* frame)

public:
    explicit frame_callback( T on_frame )
        : on_frame_function( on_frame )
    {
    }

    void on_frame( rs2_frame * fref ) override { on_frame_function( (frame_interface *)( fref ) ); }

    void release() override { delete this; }
};


template< class T >
frame_callback_ptr make_frame_callback( T callback )
{
    return { new frame_callback< T >( callback ),
             []( rs2_frame_callback * p ) { p->release(); } };
}


}  // namespace librealsense

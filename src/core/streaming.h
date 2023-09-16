// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
#pragma once

#include "options.h"
#include "../depth-sensor.h"
#include "../color-sensor.h"
#include "../composite-frame.h"
#include "../points.h"
#include "info-interface.h"
#include "tagged-profile.h"
#include "recommended-proccesing-blocks-interface.h"

#include <functional>
#include <vector>


namespace librealsense
{
    class sensor_interface;
    class device_interface;
    class processing_block_interface;

    class context;

    class stream_interface : public std::enable_shared_from_this<stream_interface>
    {
    public:
        virtual ~stream_interface() = default;

        virtual int get_stream_index() const = 0;
        virtual void set_stream_index(int index) = 0;

        virtual int get_unique_id() const = 0;
        virtual void set_unique_id(int uid) = 0;

        virtual rs2_stream get_stream_type() const = 0;
        virtual void set_stream_type(rs2_stream stream) = 0;
    };


    stream_interface * find_profile( rs2_stream stream, int index, std::vector< stream_interface * > const & profiles );


    class stream_profile_interface : public stream_interface, public recordable<stream_profile_interface>
    {
    public:
        virtual rs2_format get_format() const = 0;
        virtual void set_format(rs2_format format) = 0;

        virtual uint32_t get_framerate() const = 0;
        virtual void set_framerate(uint32_t val) = 0;

        virtual int get_tag() const = 0;
        virtual void tag_profile(int tag) = 0;

        virtual std::shared_ptr<stream_profile_interface> clone() const = 0;
        virtual rs2_stream_profile* get_c_wrapper() const = 0;
        virtual void set_c_wrapper(rs2_stream_profile* wrapper) = 0;
    };

    using on_frame = std::function<void(frame_interface*)>;
    using stream_profiles = std::vector<std::shared_ptr<stream_profile_interface>>;


    inline std::ostream& operator << (std::ostream& os, const stream_profiles& profiles)
    {
        for (auto&& p : profiles)
        {
            os << rs2_format_to_string(p->get_format()) << " " << rs2_stream_to_string(p->get_stream_type()) << ", ";
        }
        return os;
    }


    class recommended_proccesing_blocks_base : public virtual recommended_proccesing_blocks_interface, public virtual recordable<recommended_proccesing_blocks_interface>
    {
    public:
        recommended_proccesing_blocks_base(recommended_proccesing_blocks_interface* owner)
            :_owner(owner)
        {}

        virtual processing_blocks get_recommended_processing_blocks() const override { return _owner->get_recommended_processing_blocks(); };

        virtual void create_snapshot(std::shared_ptr<recommended_proccesing_blocks_interface>& snapshot) const override
        {
            snapshot = std::make_shared<recommended_proccesing_blocks_snapshot>(get_recommended_processing_blocks());
        }

        virtual void enable_recording(std::function<void(const recommended_proccesing_blocks_interface&)> recording_function)  override {}

    private:
        recommended_proccesing_blocks_interface* _owner;
    };


}

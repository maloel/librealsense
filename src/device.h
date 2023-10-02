// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "basics.h"  // C4250
#include "core/device-interface.h"
#include "core/info.h"

#include "device-info.h"

#include <rsutils/lazy.h>
#include <chrono>
#include <memory>
#include <vector>


namespace librealsense {


enum class format_conversion
{
    raw,
    basic,
    full
};


// Base implementation for most devices in librealsense. While it's not necessary to derive from this class, it greatly
// simplifies implementations.
//
class device
    : public virtual device_interface
    , public info_container
{
public:
    virtual ~device();
    size_t get_sensors_count() const override;

    sensor_interface& get_sensor(size_t subdevice) override;
    const sensor_interface& get_sensor(size_t subdevice) const override;

    void hardware_reset() override;

    virtual std::shared_ptr<matcher> create_matcher(const frame_holder& frame) const override;

    size_t find_sensor_idx(const sensor_interface& s) const;

    std::shared_ptr< context > get_context() const override { return get_device_info()->get_context(); }

    std::pair<uint32_t, rs2_extrinsics> get_extrinsics(const stream_interface& stream) const override;

    bool is_valid() const override
    {
        return get_device_info()->is_alive();
    }

    void tag_profiles(stream_profiles profiles) const override;

    virtual bool compress_while_record() const override { return true; }

    virtual bool contradicts(const stream_profile_interface* a, const std::vector<stream_profile>& others) const override;

    virtual void stop_activity() const;

    format_conversion get_format_conversion() const;

protected:
    int add_sensor(const std::shared_ptr<sensor_interface>& sensor_base);
    int assign_sensor(const std::shared_ptr<sensor_interface>& sensor_base, uint8_t idx);
    void register_stream_to_extrinsic_group(const stream_interface& stream, uint32_t groupd_index);
    std::vector<rs2_format> map_supported_color_formats(rs2_format source_format);

    device();

    std::map<int, std::pair<uint32_t, std::shared_ptr<const stream_interface>>> _extrinsics;

private:
    std::vector<std::shared_ptr<sensor_interface>> _sensors;
    mutable std::mutex _device_changed_mtx;
    rsutils::lazy< std::vector< tagged_profile > > _profiles_tags;
};


}

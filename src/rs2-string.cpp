// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#include <rscore/options-registry.h>
#include <rscore/enum-helpers.h>

#include <cassert>


const char * rs2_stream_to_string( rs2_stream stream ) { return librealsense::get_string( stream ); }
const char * rs2_format_to_string( rs2_format format ) { return librealsense::get_string( format ); }
const char * rs2_distortion_to_string( rs2_distortion distortion ) { return librealsense::get_string( distortion ); }

const char * rs2_option_to_string( rs2_option option )
{
    return librealsense::get_string( option ).c_str();
}

rs2_option rs2_option_from_string( char const * option_name )
{
    return option_name
        ? librealsense::options_registry::find_option_by_name( option_name )
        : RS2_OPTION_COUNT;
}

const char * rs2_camera_info_to_string( rs2_camera_info info ) { return librealsense::get_string( info ); }
const char * rs2_timestamp_domain_to_string( rs2_timestamp_domain info ) { return librealsense::get_string( info ); }
const char * rs2_notification_category_to_string( rs2_notification_category category ) { return librealsense::get_string( category ); }
const char * rs2_calib_target_type_to_string( rs2_calib_target_type type ) { return librealsense::get_string( type ); }
const char * rs2_sr300_visual_preset_to_string( rs2_sr300_visual_preset preset ) { return librealsense::get_string( preset ); }
const char * rs2_log_severity_to_string( rs2_log_severity severity ) { return librealsense::get_string( severity ); }
const char * rs2_exception_type_to_string( rs2_exception_type type ) { return librealsense::get_string( type ); }
const char * rs2_playback_status_to_string( rs2_playback_status status ) { return librealsense::get_string( status ); }
const char * rs2_extension_type_to_string( rs2_extension type ) { return librealsense::get_string( type ); }
const char * rs2_matchers_to_string( rs2_matchers matcher ) { return librealsense::get_string( matcher ); }
const char * rs2_frame_metadata_to_string( rs2_frame_metadata_value metadata ) { return librealsense::get_string( metadata ).c_str(); }
const char * rs2_extension_to_string( rs2_extension type ) { return rs2_extension_type_to_string( type ); }
const char * rs2_frame_metadata_value_to_string( rs2_frame_metadata_value metadata ) { return rs2_frame_metadata_to_string( metadata ); }
const char * rs2_l500_visual_preset_to_string( rs2_l500_visual_preset preset ) { return librealsense::get_string( preset ); }
const char * rs2_sensor_mode_to_string( rs2_sensor_mode mode ) { return librealsense::get_string( mode ); }
const char * rs2_ambient_light_to_string( rs2_ambient_light ambient ) { return librealsense::get_string( ambient ); }
const char * rs2_digital_gain_to_string( rs2_digital_gain gain ) { return librealsense::get_string( gain ); }
const char * rs2_cah_trigger_to_string( int mode ) { return "DEPRECATED as of 2.46"; }
const char * rs2_calibration_type_to_string( rs2_calibration_type type ) { return librealsense::get_string( type ); }
const char * rs2_calibration_status_to_string( rs2_calibration_status status ) { return librealsense::get_string( status ); }
const char * rs2_host_perf_mode_to_string( rs2_host_perf_mode mode ) { return librealsense::get_string( mode ); }
const char * rs2_emitter_frequency_mode_to_string( rs2_emitter_frequency_mode mode ) { return librealsense::get_string( mode ); }
const char * rs2_depth_auto_exposure_mode_to_string( rs2_depth_auto_exposure_mode mode ) { return librealsense::get_string( mode ); }

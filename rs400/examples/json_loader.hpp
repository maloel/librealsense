// See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

// This sample uses the advanced mode to set the algo controls of DS5.
// It can be useful for loading settings saved from IPDev.

#include <fstream>

#include <chrono>
#include <thread>
#include <sstream>
#include <map>
#include <string>

#include "json.hpp"

#include <rs400_advanced_mode/rs400_advanced_mode.hpp>

using json = nlohmann::json;

template<class T>
struct param_group
{
    using group_type = T;
    T vals[3];
    bool update = false;
};

struct stream_profile
{
    int width;
    int height;
    int fps;
    std::string depth_format;
    std::string ir_format;
};

struct laser_control
{
    std::string laser_state;
    int laser_power;
};

struct roi_control
{
    int roi_top;
    int roi_bottom;
    int roi_left;
    int roi_right;
};

struct exposure_control
{
    std::string auto_exposure;
    int exposure;
};

struct camera_state
{
    param_group<rs400::STDepthControlGroup> depth_controls;
    param_group<rs400::STRsm> rsm;
    param_group<rs400::STRauSupportVectorControl> rsvc;
    param_group<rs400::STColorControl> color_control;
    param_group<rs400::STRauColorThresholdsControl> rctc;
    param_group<rs400::STSloColorThresholdsControl> sctc;
    param_group<rs400::STSloPenaltyControl> spc;
    param_group<rs400::STHdad> hdad;
    param_group<rs400::STColorCorrection> cc;
    param_group<rs400::STDepthTableControl> depth_table;
    param_group<rs400::STAEControl> ae;
    param_group<rs400::STCensusRadius> census;
    param_group<stream_profile> profile;
    param_group<laser_control> laser;
    param_group<roi_control> roi;
    param_group<exposure_control> exposure;

    void reset()
    {
        depth_controls.update = false;
        rsm.update = false;
        rsvc.update = false;
        color_control.update = false;
        rctc.update = false;
        sctc.update = false;
        spc.update = false;
        hdad.update = false;
        cc.update = false;
        depth_table.update = false;
        ae.update = false;
        census.update = false;
        profile.update = false;
        laser.update = false;
        roi.update = false;
        exposure.update = false;

        profile.vals[0].depth_format = "Z16";
        profile.vals[0].ir_format = "COUNT";
    }
};

struct json_field
{
    virtual ~json_field() = default;

    bool was_set = false;

    virtual void load(const std::string& value) = 0;
};

template<class T, class S>
struct json_struct_field : json_field
{
    T* strct;
    S T::group_type::* field;
    float scale = 1.0f;
    bool check_ranges = true;

    void range_check(float value)
    {
        if (check_ranges)
        {
            auto min = ((strct->vals + 1)->*field);
            auto max = ((strct->vals + 2)->*field);
            if (value > max || value < min)
            {
                std::stringstream ss;
                ss << "value " << value << " is not in range [" << min << ", " << max << "]!";
                throw std::runtime_error(ss.str());
            }
        }
    }

    void load(const std::string& str) override
    {
        float value = ::atof(str.c_str());
        range_check(scale * value);
        (strct->vals[0].*field) = scale * value;
        strct->update = true;
    }
};

template<class T, class S>
struct json_string_struct_field : json_field
{
    T* strct;
    S T::group_type::* field;

    void load(const std::string& value) override
    {
        (strct->vals[0].*field) = value;
        strct->update = true;
    }
};

template<class T, class S>
struct json_invert_struct_field : json_struct_field<T, S>
{
    using json_struct_field<T, S>::range_check;
    using json_struct_field<T, S>::strct;
    using json_struct_field<T, S>::field;

    void load(const std::string& str) override
    {
        float value = ::atof(str.c_str());
        range_check(value);
        (strct->vals[0].*field) = (value > 0) ? 0 : 1;
        strct->update = true;
    }
};

template<class T, class S>
std::shared_ptr<json_field> make_field(T& strct, S T::group_type::* field, double scale = 1.0f)
{
    std::shared_ptr<json_struct_field<T, S>> f(new json_struct_field<T, S>());
    f->field = field;
    f->strct = &strct;
    f->scale = scale;
    return f;
}

template<class T, class S>
std::shared_ptr<json_field> make_string_field(T& strct, S T::group_type::* field, double scale = 1.0f)
{
    std::shared_ptr<json_string_struct_field<T, S>> f(new json_string_struct_field<T, S>());
    f->field = field;
    f->strct = &strct;
    return f;
}

template<class T, class S>
std::shared_ptr<json_field> make_invert_field(T& strct, S T::group_type::* field)
{
    std::shared_ptr<json_invert_struct_field<T, S>> f(new json_invert_struct_field<T, S>());
    f->field = field;
    f->strct = &strct;
    return f;
}


inline void load_json(const std::string& filename, camera_state& state)
{
    std::stringstream errors;
    std::stringstream warnings;

    // Read JSON file
    std::ifstream i(filename);
    json j;
    i >> j;

    std::map<std::string, std::shared_ptr<json_field>> fields = {
        // Depth Control
        { "param-leftrightthreshold", make_field(state.depth_controls, &rs400::STDepthControlGroup::lrAgreeThreshold) },
        { "param-maxscorethreshb", make_field(state.depth_controls, &rs400::STDepthControlGroup::scoreThreshB) },
        { "param-medianthreshold", make_field(state.depth_controls, &rs400::STDepthControlGroup::deepSeaMedianThreshold) },
        { "param-minscorethresha", make_field(state.depth_controls, &rs400::STDepthControlGroup::scoreThreshA) },
        { "param-neighborthresh", make_field(state.depth_controls, &rs400::STDepthControlGroup::deepSeaNeighborThreshold) },
        { "param-secondpeakdelta", make_field(state.depth_controls, &rs400::STDepthControlGroup::deepSeaSecondPeakThreshold) },
        { "param-texturecountthresh", make_field(state.depth_controls, &rs400::STDepthControlGroup::textureCountThreshold) },
        { "param-robbinsmonrodecrement", make_field(state.depth_controls, &rs400::STDepthControlGroup::minusDecrement) },
        { "param-robbinsmonroincrement", make_field(state.depth_controls, &rs400::STDepthControlGroup::plusIncrement) },

        // RSM
        { "param-usersm", make_invert_field(state.rsm, &rs400::STRsm::rsmBypass) },
        { "param-rsmdiffthreshold", make_field(state.rsm, &rs400::STRsm::diffThresh) },
        { "param-rsmrauslodiffthreshold", make_field(state.rsm, &rs400::STRsm::sloRauDiffThresh) },
        { "param-rsmremovethreshold", make_field(state.rsm, &rs400::STRsm::removeThresh, 168) },

        // RAU Support Vector Control
        { "param-raumine", make_field(state.rsvc, &rs400::STRauSupportVectorControl::minEast) },
        { "param-rauminn", make_field(state.rsvc, &rs400::STRauSupportVectorControl::minNorth) },
        { "param-rauminnssum", make_field(state.rsvc, &rs400::STRauSupportVectorControl::minNSsum) },
        { "param-raumins", make_field(state.rsvc, &rs400::STRauSupportVectorControl::minSouth) },
        { "param-rauminw", make_field(state.rsvc, &rs400::STRauSupportVectorControl::minWest) },
        { "param-rauminwesum", make_field(state.rsvc, &rs400::STRauSupportVectorControl::minWEsum) },
        { "param-regionshrinku", make_field(state.rsvc, &rs400::STRauSupportVectorControl::uShrink) },
        { "param-regionshrinkv", make_field(state.rsvc, &rs400::STRauSupportVectorControl::vShrink) },

        // Color Controls
        { "param-disableraucolor", make_field(state.color_control, &rs400::STColorControl::disableRAUColor) },
        { "param-disablesadcolor", make_field(state.color_control, &rs400::STColorControl::disableSADColor) },
        { "param-disablesadnormalize", make_field(state.color_control, &rs400::STColorControl::disableSADNormalize) },
        { "param-disablesloleftcolor", make_field(state.color_control, &rs400::STColorControl::disableSLOLeftColor) },
        { "param-disableslorightcolor", make_field(state.color_control, &rs400::STColorControl::disableSLORightColor) },

        // RAU Color Thresholds Control
        { "param-regioncolorthresholdb", make_field(state.rctc, &rs400::STRauColorThresholdsControl::rauDiffThresholdBlue, 1022) },
        { "param-regioncolorthresholdg", make_field(state.rctc, &rs400::STRauColorThresholdsControl::rauDiffThresholdGreen, 1022) },
        { "param-regioncolorthresholdr", make_field(state.rctc, &rs400::STRauColorThresholdsControl::rauDiffThresholdRed, 1022) },

        // SLO Color Thresholds Control
        { "param-scanlineedgetaub", make_field(state.sctc, &rs400::STSloColorThresholdsControl::diffThresholdBlue) },
        { "param-scanlineedgetaug", make_field(state.sctc, &rs400::STSloColorThresholdsControl::diffThresholdGreen) },
        { "param-scanlineedgetaur", make_field(state.sctc, &rs400::STSloColorThresholdsControl::diffThresholdRed) },

        // SLO Penalty Control
        { "param-scanlinep1", make_field(state.spc, &rs400::STSloPenaltyControl::sloK1Penalty) },
        { "param-scanlinep1onediscon", make_field(state.spc, &rs400::STSloPenaltyControl::sloK1PenaltyMod1) },
        { "param-scanlinep1twodiscon", make_field(state.spc, &rs400::STSloPenaltyControl::sloK1PenaltyMod2) },
        { "param-scanlinep2", make_field(state.spc, &rs400::STSloPenaltyControl::sloK2Penalty) },
        { "param-scanlinep2onediscon", make_field(state.spc, &rs400::STSloPenaltyControl::sloK2PenaltyMod1) },
        { "param-scanlinep2twodiscon", make_field(state.spc, &rs400::STSloPenaltyControl::sloK2PenaltyMod2) },

        // HDAD
        { "param-lambdaad", make_field(state.hdad, &rs400::STHdad::lambdaAD) },
        { "param-lambdacensus", make_field(state.hdad, &rs400::STHdad::lambdaCensus) },
        { "ignoreSAD", make_field(state.hdad, &rs400::STHdad::ignoreSAD) },

        // SLO Penalty Control
        { "param-colorcorrection1", make_field(state.cc, &rs400::STColorCorrection::colorCorrection1) },
        { "param-colorcorrection2", make_field(state.cc, &rs400::STColorCorrection::colorCorrection2) },
        { "param-colorcorrection3", make_field(state.cc, &rs400::STColorCorrection::colorCorrection3) },
        { "param-colorcorrection4", make_field(state.cc, &rs400::STColorCorrection::colorCorrection4) },
        { "param-colorcorrection5", make_field(state.cc, &rs400::STColorCorrection::colorCorrection5) },
        { "param-colorcorrection6", make_field(state.cc, &rs400::STColorCorrection::colorCorrection6) },
        { "param-colorcorrection7", make_field(state.cc, &rs400::STColorCorrection::colorCorrection7) },
        { "param-colorcorrection8", make_field(state.cc, &rs400::STColorCorrection::colorCorrection8) },
        { "param-colorcorrection9", make_field(state.cc, &rs400::STColorCorrection::colorCorrection9) },
        { "param-colorcorrection10", make_field(state.cc, &rs400::STColorCorrection::colorCorrection10) },
        { "param-colorcorrection11", make_field(state.cc, &rs400::STColorCorrection::colorCorrection11) },
        { "param-colorcorrection12", make_field(state.cc, &rs400::STColorCorrection::colorCorrection12) },

        // Depth Table
        { "param-depthunits", make_field(state.depth_table, &rs400::STDepthTableControl::depthUnits) },
        { "param-depthclampmin", make_field(state.depth_table, &rs400::STDepthTableControl::depthClampMin) },
        { "param-depthclampmax", make_field(state.depth_table, &rs400::STDepthTableControl::depthClampMax) },
        { "param-disparitymode", make_field(state.depth_table, &rs400::STDepthTableControl::disparityMode) },
        { "param-disparityshift", make_field(state.depth_table, &rs400::STDepthTableControl::disparityShift) },

        // Auto-Exposure
        { "param-autoexposure-setpoint", make_field(state.ae, &rs400::STAEControl::meanIntensitySetPoint) },

        // Census
        { "param-censusenablereg-udiameter", make_field(state.census, &rs400::STCensusRadius::uDiameter) },
        { "param-censusenablereg-vdiameter", make_field(state.census, &rs400::STCensusRadius::vDiameter) },

        // Stream Profile
        { "stream-width", make_field(state.profile, &stream_profile::width) },
        { "stream-height", make_field(state.profile, &stream_profile::height) },
        { "stream-fps", make_field(state.profile, &stream_profile::fps) },
        { "stream-depth-format", make_string_field(state.profile, &stream_profile::depth_format) },
        { "stream-ir-format", make_string_field(state.profile, &stream_profile::ir_format) },

        // Controls Group
        { "controls-laserstate", make_string_field(state.laser, &laser_control::laser_state) },
        { "controls-laserpower", make_field(state.laser, &laser_control::laser_power) },

        { "controls-autoexposure-roi-top", make_field(state.roi, &roi_control::roi_top) },
        { "controls-autoexposure-roi-bottom", make_field(state.roi, &roi_control::roi_bottom) },
        { "controls-autoexposure-roi-left", make_field(state.roi, &roi_control::roi_left) },
        { "controls-autoexposure-roi-right", make_field(state.roi, &roi_control::roi_right) },

        { "controls-autoexposure-auto", make_string_field(state.exposure, &exposure_control::auto_exposure) },
        { "controls-autoexposure-manual", make_field(state.exposure, &exposure_control::exposure) },
    };

    for (auto it = j.begin(); it != j.end(); ++it)
    {
        auto kvp = fields.find(it.key());
        if (kvp != fields.end())
        {
            try
            {
                if (it.value().type() != nlohmann::basic_json<>::value_t::string)
                {
                    float val = it.value();
                    std::stringstream ss;
                    ss << val;
                    kvp->second->load(ss.str());
                }
                else
                {
                    kvp->second->load(it.value());
                }
                kvp->second->was_set = true;
            }
            catch (const std::runtime_error& err)
            {
                errors << "Couldn't set \"" << it.key() << "\":" << err.what() << "\n";
            }
        }
    }

    for (auto& kvp : fields)
    {
        if (!kvp.second->was_set)
        {
            warnings << "(Warning) Parameter \"" << kvp.first << "\" was not set!\n";
        }
    }

    auto err = errors.str();
    if (err != "") throw std::runtime_error(err + warnings.str());
}

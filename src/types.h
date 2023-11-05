// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

// This header defines vocabulary types and utility mechanisms used ubiquitously by the
// rest of the library. As clearer module boundaries form, declarations might be moved
// out of this file and into more appropriate locations.

#pragma once

#include "basics.h"

#include <librealsense2/hpp/rs_types.hpp>

#include "core/enum-helpers.h"
#include <rsutils/concurrency/concurrency.h>
#include "librealsense-exception.h"
#include <rsutils/easylogging/easyloggingpp.h>
#include <rsutils/version.h>

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <cmath>                            // For acos
#include <ctime>
#include <stdint.h>
#include <cassert>                          // For assert
#include <cstring>                          // For memcmp
#include <vector>                           // For vector
#include <sstream>                          // For ostringstream
#include <mutex>                            // For mutex, unique_lock
#include <memory>                           // For unique_ptr
#include <map>
#include <limits>
#include <algorithm>
#include <condition_variable>
#include <functional>
#include <utility>                          // For std::forward
#include <limits>
#include <iomanip>

// Usage of non-standard C++ PI derivatives is prohibitive, use local definitions
static const double pi = std::acos(-1);
static const double d2r = pi / 180;
static const double r2d = 180 / pi;
template<typename T> T deg2rad(T val) { return T(val * d2r); }
template<typename T> T rad2deg(T val) { return T(val * r2d); }

#ifdef ANDROID
#include "../common/android_helpers.h"
#endif


namespace librealsense {


    static const double TIMESTAMP_USEC_TO_MSEC = 0.001;

    ///////////////////////////////////
    // Utility types for general use //
    ///////////////////////////////////


    // Comparing parameter against a range of values of the same type
    // https://stackoverflow.com/questions/15181579/c-most-efficient-way-to-compare-a-variable-to-multiple-values
    template <typename T>
    bool val_in_range(const T& val, const std::initializer_list<T>& list)
    {
        for (const auto& i : list) {
            if (val == i) {
                return true;
            }
        }
        return false;
    }


    // Enhancement for debug mode that incurs performance penalty with STL
    // std::clamp to be introduced with c++17
    template< typename T>
    inline T clamp_val(T val, const T& min, const T& max)
    {
        static_assert((std::is_arithmetic<T>::value), "clamping supports arithmetic built-in types only");
#ifdef _DEBUG
        const T t = val < min ? min : val;
        return t > max ? max : t;
#else
        return std::min(std::max(val, min), max);
#endif
    }


#pragma pack(push, 1)
    template<class T> class big_endian
    {
        T be_value;
    public:
        operator T () const
        {
            T le_value = 0;
            for (unsigned int i = 0; i < sizeof(T); ++i) *(reinterpret_cast<char*>(&le_value) + i) = *(reinterpret_cast<const char*>(&be_value) + sizeof(T) - i - 1);
            return le_value;

        }
    };
#pragma pack(pop)




    using firmware_version = rsutils::version;

    // This class is used to buffer up several writes to a structure-valued XU control, and send the entire structure all at once
    // Additionally, it will ensure that any fields not set in a given struct will retain their original values
    template<class T, class R, class W> struct struct_interface
    {
        T struct_;
        R reader;
        W writer;
        bool active;

        struct_interface(R r, W w) : reader(r), writer(w), active(false) {}

        void activate() { if (!active) { struct_ = reader(); active = true; } }
        template<class U> double get(U T::* field) { activate(); return static_cast<double>(struct_.*field); }
        template<class U, class V> void set(U T::* field, V value) { activate(); struct_.*field = static_cast<U>(value); }
        void commit() { if (active) writer(struct_); }
    };

    template<class T, class R, class W>
    std::shared_ptr<struct_interface<T, R, W>> make_struct_interface(R r, W w)
    {
        return std::make_shared<struct_interface<T, R, W>>(r, w);
    }





    bool file_exists(const char* filename);

    ///////////////////////////////////////////
    // Extrinsic auxillary routines routines //
    ///////////////////////////////////////////


    template <typename T>
    class optional_value
    {
    public:
        optional_value() : _valid(false) {}
        explicit optional_value(const T& v) : _valid(true), _value(v) {}

        operator bool() const
        {
            return has_value();
        }
        bool has_value() const
        {
            return _valid;
        }

        T& operator=(const T& v)
        {
            _valid = true;
            _value = v;
            return _value;
        }

        T& value() &
        {
            if (!_valid) throw std::runtime_error("bad optional access");
            return _value;
        }

        T&& value() &&
        {
            if (!_valid) throw std::runtime_error("bad optional access");
            return std::move(_value);
        }

        const T* operator->() const
        {
            return &_value;
        }
        T* operator->()
        {
            return &_value;
        }
        const T& operator*() const&
        {
            return _value;
        }
        T& operator*() &
        {
            return _value;
        }
        T&& operator*() &&
        {
            return std::move(_value);
        }

        bool operator==(const T& other) const
        {
            return this->_value == other;
        }

        bool operator!=(const T& other) const
        {
            return !(*this == other);
        }
    private:
        bool _valid;
        T _value;
    };
}

template<typename T>
uint32_t rs_fourcc(const T a, const T b, const  T c, const T d)
{
    static_assert((std::is_integral<T>::value), "rs_fourcc supports integral built-in types only");
    return ((static_cast<uint32_t>(a) << 24) |
            (static_cast<uint32_t>(b) << 16) |
            (static_cast<uint32_t>(c) << 8) |
            (static_cast<uint32_t>(d) << 0));
}


enum res_type {
    low_resolution,
    medium_resolution,
    high_resolution
};

inline res_type get_res_type(uint32_t width, uint32_t height)
{
    if (width == 256) // Crop resolution
        return res_type::high_resolution;

    if (width == 640)
        return res_type::medium_resolution;
    else if (width < 640)
        return res_type::low_resolution;

    return res_type::high_resolution;
}

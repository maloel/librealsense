// Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/*!
 * @file realsense_types.h
 * This header file contains the declaration of the described types in the IDL file.
 *
 * This file was generated by the tool gen.
 */

#ifndef _FAST_DDS_GENERATED_REALSENSE_REALSENSE_TYPES_H_
#define _FAST_DDS_GENERATED_REALSENSE_REALSENSE_TYPES_H_


#include <fastrtps/utils/fixed_size_string.hpp>

#include <stdint.h>
#include <array>
#include <string>
#include <vector>
#include <map>
#include <bitset>

#if defined(_WIN32)
#if defined(EPROSIMA_USER_DLL_EXPORT)
#define eProsima_user_DllExport __declspec( dllexport )
#else
#define eProsima_user_DllExport
#endif  // EPROSIMA_USER_DLL_EXPORT
#else
#define eProsima_user_DllExport
#endif  // _WIN32

#if defined(_WIN32)
#if defined(EPROSIMA_USER_DLL_EXPORT)
#if defined(realsense_types_SOURCE)
#define realsense_types_DllAPI __declspec( dllexport )
#else
#define realsense_types_DllAPI __declspec( dllimport )
#endif // realsense_types_SOURCE
#else
#define realsense_types_DllAPI
#endif  // EPROSIMA_USER_DLL_EXPORT
#else
#define realsense_types_DllAPI
#endif // _WIN32

namespace eprosima {
namespace fastcdr {
class Cdr;
} // namespace fastcdr
} // namespace eprosima


namespace realsense {
    const uint32_t DEPTH_IMAGE_WIDTH = 1280;
    const uint32_t DEPTH_IMAGE_HEIGHT = 960;
    const uint32_t DEPTH_BYTES_PER_PIXEL = 2;
    const uint32_t MAX_IMAGE_PAYLOAD_SIZE = DEPTH_IMAGE_WIDTH*DEPTH_IMAGE_HEIGHT*DEPTH_BYTES_PER_PIXEL;
    /*!
     * @brief This class represents the structure STREAM_payload defined by the user in the IDL file.
     * @ingroup REALSENSE_TYPES
     */
    class STREAM_payload
    {
    public:

        /*!
         * @brief Default constructor.
         */
        eProsima_user_DllExport STREAM_payload();

        /*!
         * @brief Default destructor.
         */
        eProsima_user_DllExport ~STREAM_payload();

        /*!
         * @brief Copy constructor.
         * @param x Reference to the object realsense::STREAM_payload that will be copied.
         */
        eProsima_user_DllExport STREAM_payload(
                const STREAM_payload& x);

        /*!
         * @brief Move constructor.
         * @param x Reference to the object realsense::STREAM_payload that will be copied.
         */
        eProsima_user_DllExport STREAM_payload(
                STREAM_payload&& x);

        /*!
         * @brief Copy assignment.
         * @param x Reference to the object realsense::STREAM_payload that will be copied.
         */
        eProsima_user_DllExport STREAM_payload& operator =(
                const STREAM_payload& x);

        /*!
         * @brief Move assignment.
         * @param x Reference to the object realsense::STREAM_payload that will be copied.
         */
        eProsima_user_DllExport STREAM_payload& operator =(
                STREAM_payload&& x);

        /*!
         * @brief Comparison operator.
         * @param x realsense::STREAM_payload object to compare.
         */
        eProsima_user_DllExport bool operator ==(
                const STREAM_payload& x) const;

        /*!
         * @brief Comparison operator.
         * @param x realsense::STREAM_payload object to compare.
         */
        eProsima_user_DllExport bool operator !=(
                const STREAM_payload& x) const;

        /*!
         * @brief This function sets a value in member stream_id
         * @param _stream_id New value for member stream_id
         */
        eProsima_user_DllExport void stream_id(
                uint64_t _stream_id);

        /*!
         * @brief This function returns the value of member stream_id
         * @return Value of member stream_id
         */
        eProsima_user_DllExport uint64_t stream_id() const;

        /*!
         * @brief This function returns a reference to member stream_id
         * @return Reference to member stream_id
         */
        eProsima_user_DllExport uint64_t& stream_id();

        /*!
         * @brief This function sets a value in member frame_number
         * @param _frame_number New value for member frame_number
         */
        eProsima_user_DllExport void frame_number(
                uint64_t _frame_number);

        /*!
         * @brief This function returns the value of member frame_number
         * @return Value of member frame_number
         */
        eProsima_user_DllExport uint64_t frame_number() const;

        /*!
         * @brief This function returns a reference to member frame_number
         * @return Reference to member frame_number
         */
        eProsima_user_DllExport uint64_t& frame_number();

        /*!
         * @brief This function copies the value in member payload
         * @param _payload New value to be copied in member payload
         */
        eProsima_user_DllExport void payload(
                const std::vector<uint8_t>& _payload);

        /*!
         * @brief This function moves the value in member payload
         * @param _payload New value to be moved in member payload
         */
        eProsima_user_DllExport void payload(
                std::vector<uint8_t>&& _payload);

        /*!
         * @brief This function returns a constant reference to member payload
         * @return Constant reference to member payload
         */
        eProsima_user_DllExport const std::vector<uint8_t>& payload() const;

        /*!
         * @brief This function returns a reference to member payload
         * @return Reference to member payload
         */
        eProsima_user_DllExport std::vector<uint8_t>& payload();

        /*!
         * @brief This function returns the maximum serialized size of an object
         * depending on the buffer alignment.
         * @param current_alignment Buffer alignment.
         * @return Maximum serialized size.
         */
        eProsima_user_DllExport static size_t getMaxCdrSerializedSize(
                size_t current_alignment = 0);

        /*!
         * @brief This function returns the serialized size of a data depending on the buffer alignment.
         * @param data Data which is calculated its serialized size.
         * @param current_alignment Buffer alignment.
         * @return Serialized size.
         */
        eProsima_user_DllExport static size_t getCdrSerializedSize(
                const realsense::STREAM_payload& data,
                size_t current_alignment = 0);


        /*!
         * @brief This function serializes an object using CDR serialization.
         * @param cdr CDR serialization object.
         */
        eProsima_user_DllExport void serialize(
                eprosima::fastcdr::Cdr& cdr) const;

        /*!
         * @brief This function deserializes an object using CDR serialization.
         * @param cdr CDR serialization object.
         */
        eProsima_user_DllExport void deserialize(
                eprosima::fastcdr::Cdr& cdr);



        /*!
         * @brief This function returns the maximum serialized size of the Key of an object
         * depending on the buffer alignment.
         * @param current_alignment Buffer alignment.
         * @return Maximum serialized size.
         */
        eProsima_user_DllExport static size_t getKeyMaxCdrSerializedSize(
                size_t current_alignment = 0);

        /*!
         * @brief This function tells you if the Key has been defined for this type
         */
        eProsima_user_DllExport static bool isKeyDefined();

        /*!
         * @brief This function serializes the key members of an object using CDR serialization.
         * @param cdr CDR serialization object.
         */
        eProsima_user_DllExport void serializeKey(
                eprosima::fastcdr::Cdr& cdr) const;

    private:

        uint64_t m_stream_id;
        uint64_t m_frame_number;
        std::vector<uint8_t> m_payload;
    };
    /*!
     * @brief This class represents the structure OP_payload defined by the user in the IDL file.
     * @ingroup REALSENSE_TYPES
     */
    class OP_payload
    {
    public:

        /*!
         * @brief Default constructor.
         */
        eProsima_user_DllExport OP_payload();

        /*!
         * @brief Default destructor.
         */
        eProsima_user_DllExport ~OP_payload();

        /*!
         * @brief Copy constructor.
         * @param x Reference to the object realsense::OP_payload that will be copied.
         */
        eProsima_user_DllExport OP_payload(
                const OP_payload& x);

        /*!
         * @brief Move constructor.
         * @param x Reference to the object realsense::OP_payload that will be copied.
         */
        eProsima_user_DllExport OP_payload(
                OP_payload&& x);

        /*!
         * @brief Copy assignment.
         * @param x Reference to the object realsense::OP_payload that will be copied.
         */
        eProsima_user_DllExport OP_payload& operator =(
                const OP_payload& x);

        /*!
         * @brief Move assignment.
         * @param x Reference to the object realsense::OP_payload that will be copied.
         */
        eProsima_user_DllExport OP_payload& operator =(
                OP_payload&& x);

        /*!
         * @brief Comparison operator.
         * @param x realsense::OP_payload object to compare.
         */
        eProsima_user_DllExport bool operator ==(
                const OP_payload& x) const;

        /*!
         * @brief Comparison operator.
         * @param x realsense::OP_payload object to compare.
         */
        eProsima_user_DllExport bool operator !=(
                const OP_payload& x) const;

        /*!
         * @brief This function sets a value in member op
         * @param _op New value for member op
         */
        eProsima_user_DllExport void op(
                uint64_t _op);

        /*!
         * @brief This function returns the value of member op
         * @return Value of member op
         */
        eProsima_user_DllExport uint64_t op() const;

        /*!
         * @brief This function returns a reference to member op
         * @return Reference to member op
         */
        eProsima_user_DllExport uint64_t& op();

        /*!
         * @brief This function sets a value in member id
         * @param _id New value for member id
         */
        eProsima_user_DllExport void id(
                uint64_t _id);

        /*!
         * @brief This function returns the value of member id
         * @return Value of member id
         */
        eProsima_user_DllExport uint64_t id() const;

        /*!
         * @brief This function returns a reference to member id
         * @return Reference to member id
         */
        eProsima_user_DllExport uint64_t& id();

        /*!
         * @brief This function copies the value in member data
         * @param _data New value to be copied in member data
         */
        eProsima_user_DllExport void data(
                const std::array<uint64_t, 5>& _data);

        /*!
         * @brief This function moves the value in member data
         * @param _data New value to be moved in member data
         */
        eProsima_user_DllExport void data(
                std::array<uint64_t, 5>&& _data);

        /*!
         * @brief This function returns a constant reference to member data
         * @return Constant reference to member data
         */
        eProsima_user_DllExport const std::array<uint64_t, 5>& data() const;

        /*!
         * @brief This function returns a reference to member data
         * @return Reference to member data
         */
        eProsima_user_DllExport std::array<uint64_t, 5>& data();

        /*!
         * @brief This function returns the maximum serialized size of an object
         * depending on the buffer alignment.
         * @param current_alignment Buffer alignment.
         * @return Maximum serialized size.
         */
        eProsima_user_DllExport static size_t getMaxCdrSerializedSize(
                size_t current_alignment = 0);

        /*!
         * @brief This function returns the serialized size of a data depending on the buffer alignment.
         * @param data Data which is calculated its serialized size.
         * @param current_alignment Buffer alignment.
         * @return Serialized size.
         */
        eProsima_user_DllExport static size_t getCdrSerializedSize(
                const realsense::OP_payload& data,
                size_t current_alignment = 0);


        /*!
         * @brief This function serializes an object using CDR serialization.
         * @param cdr CDR serialization object.
         */
        eProsima_user_DllExport void serialize(
                eprosima::fastcdr::Cdr& cdr) const;

        /*!
         * @brief This function deserializes an object using CDR serialization.
         * @param cdr CDR serialization object.
         */
        eProsima_user_DllExport void deserialize(
                eprosima::fastcdr::Cdr& cdr);



        /*!
         * @brief This function returns the maximum serialized size of the Key of an object
         * depending on the buffer alignment.
         * @param current_alignment Buffer alignment.
         * @return Maximum serialized size.
         */
        eProsima_user_DllExport static size_t getKeyMaxCdrSerializedSize(
                size_t current_alignment = 0);

        /*!
         * @brief This function tells you if the Key has been defined for this type
         */
        eProsima_user_DllExport static bool isKeyDefined();

        /*!
         * @brief This function serializes the key members of an object using CDR serialization.
         * @param cdr CDR serialization object.
         */
        eProsima_user_DllExport void serializeKey(
                eprosima::fastcdr::Cdr& cdr) const;

    private:

        uint64_t m_op;
        uint64_t m_id;
        std::array<uint64_t, 5> m_data;
    };
} // namespace realsense

#endif // _FAST_DDS_GENERATED_REALSENSE_REALSENSE_TYPES_H_
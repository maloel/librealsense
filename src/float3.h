// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.
#pragma once

#include <ostream>
#include <cassert>


namespace librealsense {


////////////////////////////////////////////
// World's tiniest linear algebra library //
////////////////////////////////////////////


#pragma pack( push, 1 )
struct int2
{
    int x, y;
};
struct float2
{
    float x, y;
    float & operator[]( int i )
    {
        assert( i >= 0 );
        assert( i < 2 );
        return *( &x + i );
    }
};
struct float3
{
    float x, y, z;
    float & operator[]( int i )
    {
        assert( i >= 0 );
        assert( i < 3 );
        return ( *( &x + i ) );
    }
};
struct float4
{
    float x, y, z, w;
    float & operator[]( int i )
    {
        assert( i >= 0 );
        assert( i < 4 );
        return ( *( &x + i ) );
    }
};
struct float3x3
{
    float3 x, y, z;
    float & operator()( int i, int j )
    {
        assert( i >= 0 );
        assert( i < 3 );
        assert( j >= 0 );
        assert( j < 3 );
        return ( *( &x[0] + j * sizeof( float3 ) / sizeof( float ) + i ) );
    }
};  // column-major
#pragma pack( pop )

inline bool operator==( const float3 & a, const float3 & b )
{
    return a.x == b.x && a.y == b.y && a.z == b.z;
}
inline float3 operator+( const float3 & a, const float3 & b )
{
    return { a.x + b.x, a.y + b.y, a.z + b.z };
}
inline float3 operator-( const float3 & a, const float3 & b )
{
    return { a.x - b.x, a.y - b.y, a.z - b.z };
}
inline float3 operator*( const float3 & a, float b )
{
    return { a.x * b, a.y * b, a.z * b };
}
inline bool operator==( const float4 & a, const float4 & b )
{
    return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}
inline float4 operator+( const float4 & a, const float4 & b )
{
    return { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
}
inline float4 operator-( const float4 & a, const float4 & b )
{
    return { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w };
}
inline bool operator==( const float3x3 & a, const float3x3 & b )
{
    return a.x == b.x && a.y == b.y && a.z == b.z;
}
inline float3 operator*( const float3x3 & a, const float3 & b )
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
inline float3x3 operator*( const float3x3 & a, const float3x3 & b )
{
    return { a * b.x, a * b.y, a * b.z };
}
inline float3x3 transpose( const float3x3 & a )
{
    return { { a.x.x, a.y.x, a.z.x }, { a.x.y, a.y.y, a.z.y }, { a.x.z, a.y.z, a.z.z } };
}

inline std::ostream & operator<<( std::ostream & stream, const float3 & elem )
{
    return stream << elem.x << " " << elem.y << " " << elem.z;
}
inline std::ostream & operator<<( std::ostream & stream, const float4 & elem )
{
    return stream << elem.x << " " << elem.y << " " << elem.z << " " << elem.w;
}
inline std::ostream & operator<<( std::ostream & stream, const float3x3 & elem )
{
    return stream << elem.x << "\n" << elem.y << "\n" << elem.z;
}


}  // namespace librealsense

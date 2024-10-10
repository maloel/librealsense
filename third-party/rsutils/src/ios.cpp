// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#include <rsutils/ios/field.h>
#include <rsutils/ios/indent.h>
#include <iostream>


namespace rsutils {
namespace ios {


long & indent_flag( std::ios_base & s )
{
    static int const index = std::ios_base::xalloc();
    return s.iword( index );
}


static long & first_flag( std::ios_base & s )
{
    static int const index = std::ios_base::xalloc();
    return s.iword( index );
}


inline bool is_first( std::ios_base & s ) { return first_flag( s ) > 0; }
inline bool set_first( std::ios_base & s, bool f = true )
{
    bool was_first = is_first( s );
    first_flag( s ) = (long)f;
    return was_first;
}


std::ostream & operator<<( std::ostream & os, indent const & indent )
{
    add_indent( os, indent.d );
    return os;
}


/*static*/ std::ostream & field::sameline( std::ostream & os )
{
    if( ! set_first( os, false ) )
        os << ' ';
    return os;
}


/*static*/ std::ostream & field::separator( std::ostream & os )
{
    if( int i = get_indent( os ) )
    {
        os << '\n';
        while( i-- )
            os << ' ';
        set_first( os, true );
    }
    else
    {
        sameline( os );
    }
    return os;
}


/*static*/ std::ostream & field::value( std::ostream & os )
{
    if( ! set_first( os, false ) )
        os << ' ';
    return os;
}


/*static*/ std::ostream & field::first( std::ostream & os )
{
    set_first( os, true );
    return os;
}


/*static*/ std::ostream & field::group::start( std::ostream & os )
{
    os << '[';
    set_first( os, true );
    return os;
}


/*static*/ std::ostream & field::group::end( std::ostream & os )
{
    os << ']';
    set_first( os, false );
    return os;
}


std::ostream & operator<<( std::ostream & os, field::group const & group )
{
    if( has_indent( os ) )
        os << indent();
    else
        os << field::group::start;
    group.pos = &os;
    return os;
}


field::group::~group()
{
    if( pos )
    {
        if( has_indent( *pos ) )
            *pos << unindent();
        else
            *pos << field::group::end;
    }
}


}
}  // namespace rsutils

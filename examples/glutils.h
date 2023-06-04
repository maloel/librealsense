#pragma once

#include <GLFW/glfw3.h>
#include <vector>
#include "rect.h"
#include <third-party/stb_easy_font.h>


inline void draw_text( int x, int y, const char * text )
{
    std::vector<char> buffer;
    buffer.resize( 60000 ); // ~300 chars
    glEnableClientState( GL_VERTEX_ARRAY );
    glVertexPointer( 2, GL_FLOAT, 16, &(buffer[0]) );
    glDrawArrays( GL_QUADS,
                  0,
                  4
                  * stb_easy_font_print( (float) x,
                                         (float) (y - 7),
                                         (char *) text,
                                         nullptr,
                                         &(buffer[0]),
                                         int( sizeof( char ) * buffer.size() ) ) );
    glDisableClientState( GL_VERTEX_ARRAY );
}


void set_viewport( const rect & r )
{
    glViewport( (int) r.x, (int) r.y, (int) r.w, (int) r.h );
    glLoadIdentity();
    glMatrixMode( GL_PROJECTION );
    glOrtho( 0, r.w, r.h, 0, -1, +1 );
}


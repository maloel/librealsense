// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

//#include "example.hpp"
#include "HostEvent.h"
#include <examples/window.h>
#include <examples/rect.h>
#include <examples/glutils.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <iostream>


class texture
{
public:

    void upload( const CamSetting & frame )
    {
        if( !frame.buf ) return;

        if( !_gl_handle )
            glGenTextures( 1, &_gl_handle );
        GLenum err = glGetError();

        //auto format = frame.get_profile().format();
        auto width = frame.width;
        auto height = frame.height;
        _name = frame.name;
        //_stream_index = frame.get_profile().stream_index();

        glBindTexture( GL_TEXTURE_2D, _gl_handle );

#if 1
        glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, frame.buf );
#else
        switch( format )
        {
        case RS2_FORMAT_RGB8:
            glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, frame.get_data() );
            break;
        case RS2_FORMAT_RGBA8:
            glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, frame.get_data() );
            break;
        case RS2_FORMAT_Y8:
            glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, frame.get_data() );
            break;
        case RS2_FORMAT_Y10BPACK:
            glTexImage2D( GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_SHORT, frame.get_data() );
            break;
        default:
            throw std::runtime_error( "The requested format is not supported by this demo!" );
        }
#endif

        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
        glPixelStorei( GL_UNPACK_ROW_LENGTH, 0 );
        glBindTexture( GL_TEXTURE_2D, 0 );
    }

    void show( const rect & r, float alpha = 1.f ) const
    {
        if( !_gl_handle )
            return;

        set_viewport( r );

        glBindTexture( GL_TEXTURE_2D, _gl_handle );
        glColor4f( 1.0f, 1.0f, 1.0f, alpha );
        glEnable( GL_TEXTURE_2D );
        glBegin( GL_QUADS );
        glTexCoord2f( 0, 0 ); glVertex2f( 0, 0 );
        glTexCoord2f( 0, 1 ); glVertex2f( 0, r.h );
        glTexCoord2f( 1, 1 ); glVertex2f( r.w, r.h );
        glTexCoord2f( 1, 0 ); glVertex2f( r.w, 0 );
        glEnd();
        glDisable( GL_TEXTURE_2D );
        glBindTexture( GL_TEXTURE_2D, 0 );
        draw_text( int( 0.05f * r.w ), int( 0.05f * r.h ), _name );
    }

    GLuint get_gl_handle() { return _gl_handle; }

    void render( const CamSetting & frame, const rect & rect, float alpha = 1.f )
    {
        upload( frame );
        show( rect.adjust_ratio( { (float) frame.width, (float) frame.height } ), alpha );
    }

private:
    GLuint          _gl_handle = 0;
    char const *    _name;
};



// This example assumes camera with depth and color
// streams, and direction lets you define the target stream
enum class direction
{
    to_depth,
    to_color
};

// Forward definition of UI rendering, implemented below
void render_slider( rect location, float * alpha );

HostEvent he;
CamSetting rgb_cam( RGB_CAMERA, "RGB", 640, 480, 30 );
CamSetting depth_cam( DEPTH_CAMERA, "DEPTH", 640, 480, 30 );

int main( int argc, char * argv[] ) try
{
    // Create and initialize GUI related objects
    window app( 1280, 720, "HKR DDS Test" ); // Simple window handling
    ImGui_ImplGlfw_Init( app, false );      // ImGui library intializition
    texture depth_image, color_image;     // Helpers for renderig images

    float       alpha = 0.5f;               // Transparancy coefficient
    direction   dir = direction::to_depth;  // Alignment direction

    he.init( &rgb_cam );
    he.init( &depth_cam );
    he.initDDS();

    while( app ) // Application still alive?
    {
        he.waitForFrame();

        glEnable( GL_BLEND );
        // Use the Alpha channel for blending
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

        if( dir == direction::to_depth )
        {
            // When aligning to depth, first render depth image
            // and then overlay color on top with transparancy
            depth_image.render( depth_cam, { 0, 0, app.width(), app.height() } );
            color_image.render( rgb_cam, { 0, 0, app.width(), app.height() }, alpha );
        }
        else
        {
            // When aligning to color, first render color image
            // and then overlay depth image on top
            color_image.render( rgb_cam, { 0, 0, app.width(), app.height() } );
            depth_image.render( depth_cam, { 0, 0, app.width(), app.height() }, 1 - alpha );
        }

        glColor4f( 1.f, 1.f, 1.f, 1.f );
        glDisable( GL_BLEND );

        // Render the UI:
        ImGui_ImplGlfw_NewFrame( 1 );
        render_slider( { 15.f, app.height() - 60, app.width() - 30, app.height() }, &alpha );
        ImGui::Render();
    }

    return EXIT_SUCCESS;
}
catch( const std::exception & e )
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}

void render_slider( rect location, float * alpha )
{
    static const int flags = ImGuiWindowFlags_NoCollapse
        | ImGuiWindowFlags_NoScrollbar
        | ImGuiWindowFlags_NoSavedSettings
        | ImGuiWindowFlags_NoTitleBar
        | ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_NoMove;

    ImGui::SetNextWindowPos( { location.x, location.y } );
    ImGui::SetNextWindowSize( { location.w, location.h } );

    // Render transparency slider:
    ImGui::Begin( "slider", nullptr, flags );
    ImGui::PushItemWidth( -1 );
    ImGui::SliderFloat( "##Slider", alpha, 0.f, 1.f );
    ImGui::PopItemWidth();
    if( ImGui::IsItemHovered() )
        ImGui::SetTooltip( "Texture Transparancy: %.3f", *alpha );

    if( ImGui::Checkbox( "Depth", &depth_cam.start ) )
    {
        he.sendHostEvent( &depth_cam );
    }
    ImGui::SameLine();
    ImGui::SetCursorPosX( location.w - 140 );
    if( ImGui::Checkbox( "Color", &rgb_cam.start ) )
    {
        he.sendHostEvent( &rgb_cam );
    }

    ImGui::End();
}

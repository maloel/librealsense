#pragma once

#include <GLFW/glfw3.h>


class window
{
public:
    std::function<void( bool )>           on_left_mouse = []( bool ) {};
    std::function<void( double, double )> on_mouse_scroll = []( double, double ) {};
    std::function<void( double, double )> on_mouse_move = []( double, double ) {};
    std::function<void( int )>            on_key_release = []( int ) {};

    window( int width, int height, const char * title )
        : _width( width ), _height( height ), _canvas_left_top_x( 0 ), _canvas_left_top_y( 0 ), _canvas_width( width ), _canvas_height( height )
    {
        glfwInit();
        win = glfwCreateWindow( width, height, title, nullptr, nullptr );
        if( !win )
            throw std::runtime_error( "Could not open OpenGL window, please check your graphic drivers or use the textual SDK tools" );
        glfwMakeContextCurrent( win );

        glfwSetWindowUserPointer( win, this );
        glfwSetMouseButtonCallback( win, []( GLFWwindow * w, int button, int action, int mods )
                                    {
                                        auto s = (window *) glfwGetWindowUserPointer( w );
                                        if( button == 0 ) s->on_left_mouse( action == GLFW_PRESS );
                                    } );

        glfwSetScrollCallback( win, []( GLFWwindow * w, double xoffset, double yoffset )
                               {
                                   auto s = (window *) glfwGetWindowUserPointer( w );
                                   s->on_mouse_scroll( xoffset, yoffset );
                               } );

        glfwSetCursorPosCallback( win, []( GLFWwindow * w, double x, double y )
                                  {
                                      auto s = (window *) glfwGetWindowUserPointer( w );
                                      s->on_mouse_move( x, y );
                                  } );

        glfwSetKeyCallback( win, []( GLFWwindow * w, int key, int scancode, int action, int mods )
                            {
                                auto s = (window *) glfwGetWindowUserPointer( w );
                                if( 0 == action ) // on key release
                                {
                                    s->on_key_release( key );
                                }
                            } );
    }

    //another c'tor for adjusting specific frames in specific tiles, this window is NOT resizeable
    window( unsigned width, unsigned height, const char * title, unsigned tiles_in_row, unsigned tiles_in_col, float canvas_width = 0.8f,
            float canvas_height = 0.6f, float canvas_left_top_x = 0.1f, float canvas_left_top_y = 0.075f )
        : _width( width ), _height( height ), _tiles_in_row( tiles_in_row ), _tiles_in_col( tiles_in_col )

    {
        //user input verification for mosaic size, if invalid values were given - set to default
        if( canvas_width < 0 || canvas_width > 1 || canvas_height < 0 || canvas_height > 1 ||
            canvas_left_top_x < 0 || canvas_left_top_x > 1 || canvas_left_top_y < 0 || canvas_left_top_y > 1 )
        {
            std::cout << "Invalid window's size parameter entered, setting to default values" << std::endl;
            canvas_width = 0.8f;
            canvas_height = 0.6f;
            canvas_left_top_x = 0.15f;
            canvas_left_top_y = 0.075f;
        }

        //user input verification for number of tiles in row and column
        if( _tiles_in_row <= 0 ) {
            _tiles_in_row = 4;
        }
        if( _tiles_in_col <= 0 ) {
            _tiles_in_col = 2;
        }

        //calculate canvas size
        _canvas_width = int( _width * canvas_width );
        _canvas_height = int( _height * canvas_height );
        _canvas_left_top_x = _width * canvas_left_top_x;
        _canvas_left_top_y = _height * canvas_left_top_y;

        //calculate tile size
        _tile_width_pixels = float( std::floor( _canvas_width / _tiles_in_row ) );
        _tile_height_pixels = float( std::floor( _canvas_height / _tiles_in_col ) );

        glfwInit();
        // we don't want to enable resizing the window
        glfwWindowHint( GLFW_RESIZABLE, GL_FALSE );
        win = glfwCreateWindow( width, height, title, nullptr, nullptr );
        if( !win )
            throw std::runtime_error( "Could not open OpenGL window, please check your graphic drivers or use the textual SDK tools" );
        glfwMakeContextCurrent( win );

        glfwSetWindowUserPointer( win, this );
        glfwSetMouseButtonCallback( win, []( GLFWwindow * w, int button, int action, int mods )
                                    {
                                        auto s = (window *) glfwGetWindowUserPointer( w );
                                        if( button == 0 ) s->on_left_mouse( action == GLFW_PRESS );
                                    } );

        glfwSetScrollCallback( win, []( GLFWwindow * w, double xoffset, double yoffset )
                               {
                                   auto s = (window *) glfwGetWindowUserPointer( w );
                                   s->on_mouse_scroll( xoffset, yoffset );
                               } );

        glfwSetCursorPosCallback( win, []( GLFWwindow * w, double x, double y )
                                  {
                                      auto s = (window *) glfwGetWindowUserPointer( w );
                                      s->on_mouse_move( x, y );
                                  } );

        glfwSetKeyCallback( win, []( GLFWwindow * w, int key, int scancode, int action, int mods )
                            {
                                auto s = (window *) glfwGetWindowUserPointer( w );
                                if( 0 == action ) // on key release
                                {
                                    s->on_key_release( key );
                                }
                            } );
    }

    ~window()
    {
        glfwDestroyWindow( win );
        glfwTerminate();
    }

    void close()
    {
        glfwSetWindowShouldClose( win, 1 );
    }

    float width() const { return float( _width ); }
    float height() const { return float( _height ); }

    operator bool()
    {
        glPopMatrix();
        glfwSwapBuffers( win );

        auto res = !glfwWindowShouldClose( win );

        glfwPollEvents();
        glfwGetFramebufferSize( win, &_width, &_height );

        // Clear the framebuffer
        glClear( GL_COLOR_BUFFER_BIT );
        glViewport( 0, 0, _width, _height );

        // Draw the images
        glPushMatrix();
        glfwGetWindowSize( win, &_width, &_height );
        glOrtho( 0, _width, _height, 0, -1, +1 );

        return res;
    }


    operator GLFWwindow * () { return win; }

private:
    GLFWwindow * win;
    int _width, _height;
    float _canvas_left_top_x, _canvas_left_top_y;
    int _canvas_width, _canvas_height;
    unsigned _tiles_in_row, _tiles_in_col;
    float _tile_width_pixels, _tile_height_pixels;
};


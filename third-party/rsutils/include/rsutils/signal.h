// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <vector>
#include <mutex>
#include <functional>


namespace rsutils {


// Outside signal<> so can be easy to reference - they should be the same regardless of the type of signal
using signal_slot = int;


// Signals are callbacks with multiple targets.
// 
// Each "slot" is a subscriber that's called when the signal is "raised."
// 
// There are intentionally no operators here (especially operator()!) as, despite the nice-to-look-at style, are not
// verbose enough to be really useful.
// 
// The template Args are the callback arguments, and therefore what's expected to be passed to raise(). The callbacks
// return void on purpose, for simplicity.
//
template< typename... Args >
class signal
{
    using callback = std::function< void( Args... ) >;

    std::mutex _mutex;
    std::vector< callback > _subscribers;
    signal_slot _free_slot = -1;
    size_t _size = 0;

    signal( const signal & other ) = delete;
    signal & operator=( const signal & ) = delete;

public:
    signal() = default;

    signal( signal && other )
    {
        std::lock_guard< std::mutex > locker( other._mutex );
        _subscribers = std::move( other._subscribers );
        _free_slot = other._free_slot;
        _size = other._size;
    }

    signal & operator=( signal && other )
    {
        std::lock_guard< std::mutex > locker( other._mutex );
        _subscribers = std::move( other._subscribers );
        _free_slot = other._free_slot;
        _size = other._size;
        return *this;
    }

    signal_slot subscribe( const callback && func )
    {
        if( ! func )
            return -1;
        std::lock_guard< std::mutex > locker( _mutex );
        signal_slot slot_id = _free_slot;
        if( slot_id >= 0 )
        {
            _subscribers[_free_slot] = std::move( func );
            _free_slot = -1;
        }
        else
        {
            slot_id = int( _subscribers.size() );
            _subscribers.emplace_back( std::move( func ) );
        }
        ++_size;
        return slot_id;
    }

    bool unsubscribe( signal_slot token )
    {
        std::lock_guard< std::mutex > locker( _mutex );
        if( token < 0 || token >= _subscribers.size() )
            return false;  // Bad slot
        auto & slot = _subscribers[token];
        if( ! slot )
            return false;  // Unsubscribed...
        if( ! --_size )
        {
            _subscribers.clear();
            _free_slot = -1;
        }
        else
        {
            slot = {};           // empty function -> no subscriber
            _free_slot = token;  // Next insert can go in this slot...
        }
        return true;
    }

    void raise( Args... args )
    {
        std::vector< callback > functions;

        {
            std::lock_guard< std::mutex > locker( _mutex );
            functions.reserve( _size );
            for( auto i = 0; i < _subscribers.size(); ++i )
            {
                auto const & slot = _subscribers[i];
                if( slot )
                    functions.push_back( slot );
                else if( _free_slot < 0 )
                    _free_slot = i;
            }
        }

        // NOTE: when calling our subscribers, we do not perfectly forward on purpose to avoid the situation where the
        // first subscriber will move an argument and the second will then get nothing!
        //
        for( auto const & func : functions )
            func( /*std::forward< Args >(*/ args /*)*/... );
    }

    // How many subscriptions are active
    size_t size() const { return _size; }
};


// This is what was called 'signal' before: it is a way to expose the signal as an actual public interface from your
// class. Rather than:
//      class X
//      {
//          signal< ... > _signal;
// 
//      public:
//          int set_callback( std::function< ... > && callback ) { return _signal.subscribe( std::move( callback ); }
//      };
// You can write:
//      class X
//      {
//      public:
//          public_signal< X, ... > callbacks;
//      };
// And:
//      X x;
//      x.callbacks.subscribe( []() { ... } );
// 
// The client (the one using X) should be able to subscribe/ubsubscribe, but not to raise callbacks. This is done by
// adding access specifiers such that raising is private and only a friend 'HostingClass' can raise:
//
template< typename HostingClass, typename... Args >
class public_signal : public signal< Args... >
{
    typedef signal< Args... > super;

public:
    using super::subscribe;
    using super::unsubscribe;

private:
    friend HostingClass;

    using super::raise;
};


}  // namespace rsutils

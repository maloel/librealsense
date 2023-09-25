// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include <rsutils/signal.h>


namespace {


bool function_slot_triggered = false;
bool static_slot_triggered = false;
bool functor_slot_triggered = false;


void function_slot()
{
    function_slot_triggered = true;
}


struct class_test
{
    static void static_slot() { static_slot_triggered = true; }
    bool bound_slot_triggered = false;
    void bound_slot() { bound_slot_triggered = true; }
    void operator()() { functor_slot_triggered = true; }
};


std::string _testing;
void describe( std::string const & description, std::function< void() > && how )
{
    _testing = description;
    how();
}


void it( std::string const & description, std::function< void() > && how )
{
    std::cout << "-I-     " << _testing << ' ' << description << std::endl;
    how();
}


bool _failed_signal = false;


#define test_that( VAR, OP, PRED )                                                                                     \
    if( (VAR) OP (PRED) )                                                                                              \
        /*std::cout << "    " << #VAR << ' ' << #PRED << std::endl*/;                                                  \
    else                                                                                                               \
        _failed_signal = true,                                                                                         \
        std::cout << "-E-  x    " << #VAR << " (" << (VAR) << ") " << #OP << " (" << (PRED) << ") " << #PRED << std::endl


bool test_signal()
{
    using signal_t = rsutils::signal<>;
    // using connection = typename rsutils::signal<>::connection;

    describe( "signal",
              []
              {
                  it( "should trigger basic function slots",
                      []
                      {
                          signal_t signal;
                          signal.subscribe( &function_slot );
                          signal.raise();
                          test_that( function_slot_triggered, ==, true );
                      } );

                  it( "should trigger static method slots",
                      []
                      {
                          signal_t signal;
                          signal.subscribe( &class_test::static_slot );
                          signal.raise();
                          test_that( static_slot_triggered, ==, true );
                      } );
                  it( "should trigger bound member function slots",
                      []
                      {
                          class_test obj;
                          signal_t signal;
                          signal.subscribe( std::bind( &class_test::bound_slot, &obj ) );
                          signal.raise();
                          test_that( obj.bound_slot_triggered, ==, true );
                      } );
                  it( "should trigger functor slots",
                      []
                      {
                          class_test obj;
                          signal_t signal;
                          signal.subscribe( obj );
                          signal.raise();
                          test_that( functor_slot_triggered, ==, true );
                      } );
                  it( "should trigger lambda slots",
                      []
                      {
                          bool fired = false;
                          signal_t signal;
                          signal.subscribe( [&] { fired = true; } );
                          signal.raise();
                          test_that( fired, ==, true );
                      } );
              } );
    describe( "raise()",
              []
              {
                  it( "should NOT perfectly forward r-value references",
                      []
                      {
                          // Explanation: passing r-value refs (&&) will cause the first slot to be constructed
                          // with it -- essentially moving it -- and the second slot to get nothing!
                          rsutils::signal< std::string > signal;
                          std::string str( "hello world" );
                          signal.subscribe( []( std::string str ) { test_that( str, ==, "hello world" ); } );
                          signal.subscribe( []( std::string str ) { test_that( str, ==, "hello world" ); } );
                          signal.raise( std::move( str ) );
                      } );
                  it( "should not copy references",
                      []
                      {
                          rsutils::signal< std::string & > signal;
                          std::string str( "hello world" );
                          signal.subscribe(
                              []( std::string & str )
                              {
                                  test_that( str, ==, "hello world" );
                                  str = "hola mundo";
                              } );
                          signal.subscribe( []( std::string & str ) { test_that( str, ==, "hola mundo" ); } );
                          signal.raise( str );
                      } );
                  it( "should be re-entrant",
                      []
                      {
                          unsigned count = 0;
                          signal_t signal;
                          signal.subscribe(
                              [&]
                              {
                                  ++count;
                                  if( count == 1 )
                                  {
                                      signal.subscribe( [&] { ++count; } );
                                      signal.raise();
                                  };
                              } );
                          signal.raise();
                          test_that( count, ==, 3 );
                      } );
              } );
    describe( "size()",
              []
              {
                  it( "should return the subscription count",
                      []
                      {
                          signal_t signal;
                          signal.subscribe( [] {} );
                          test_that( signal.size(), ==, 1 );
                      } );
                  it( "should return the correct count when adding slots during iteration",
                      []
                      {
                          signal_t signal;
                          signal.subscribe(
                              [&]
                              {
                                  signal.subscribe( [] {} );
                                  test_that( signal.size(), ==, 2 );
                              } );
                          signal.raise();
                          test_that( signal.size(), ==, 2 );
                      } );
              } );
    // describe( "#connect_once()",
    //          [&]
    //          {
    //              it( "it should fire once",
    //                  [&]
    //                  {
    //                      unsigned count = 0;
    //                      signal.connect_once( [&] { count++; } );
    //                      signal.raise();
    //                      test_that( count, ==, 1 );
    //                      signal.raise();
    //                      test_that( count, ==, 1 );
    //                  } );
    //          } );
    // describe( "#disconnect_all()",
    //          [&]
    //          {
    //              /*
    //                it("should remove all slots", [&]
    //                {
    //                  auto conn1 = signal.subscribe([]{});
    //                  auto conn2 = signal.subscribe([]{});
    //                  signal.disconnect_all();
    //                  test_that(signal.size(), ==(0u));
    //                  test_that(conn1.connected(), ==(false));
    //                  test_that(conn2.connected(), ==(false));
    //                  test_that(signal.empty(), ==(true));
    //                });*/
    //              it( "should remove all slots while iterating",
    //                  [&]
    //                  {
    //                      // should still fire each slot once
    //                      // matches node.js event emitter behavior
    //                      std::pair< unsigned, connection > res1, res2;
    //                      res1.second = signal.subscribe(
    //                          [&]
    //                          {
    //                              res1.first++;
    //                              signal.disconnect_all();
    //                          } );

    //                      res2.second = signal.subscribe( [&] { res2.first++; } );
    //                      signal.raise();

    //                      test_that( signal.size(), ==( 0u ) );
    //                      test_that( res1.second.connected(), ==( false ) );
    //                      test_that( res2.second.connected(), ==( false ) );
    //                      test_that( res1.first, ==( 1 ) );
    //                      test_that( res2.first, ==( 1 ) );
    //                  } );
    //              it( "should remove all slots while iterating, without removing new slots",
    //                  [&]
    //                  {
    //                      std::pair< unsigned, connection > res1, res2, res3;
    //                      res1.second = signal.subscribe(
    //                          [&]
    //                          {
    //                              res1.first++;
    //                              signal.disconnect_all();
    //                              res3.second = signal.subscribe( [&] { res3.first++; } );
    //                          } );

    //                      res2.second = signal.subscribe( [&] { res2.first++; } );
    //                      signal.raise();
    //                      signal.raise();
    //                      test_that( signal.size(), ==( 1u ) );
    //                      test_that( res1.first, ==( 1u ) );
    //                      test_that( res2.first, ==( 1u ) );
    //                      test_that( res2.first, ==( 1u ) );
    //                  } );

    //              it( "should support disconnect_all while iterating, followed by subscribe/raise",
    //                  [&]
    //                  {
    //                      std::pair< unsigned, connection > res1, res2;
    //                      res1.second = signal.subscribe(
    //                          [&]
    //                          {
    //                              res1.first++;
    //                              signal.disconnect_all();
    //                              res2.second = signal.subscribe( [&] { res2.first++; } );
    //                              signal.raise();
    //                          } );
    //                      signal.raise();
    //                      test_that( res1.first, ==( 1u ) );
    //                      test_that( res2.first, ==( 1u ) );
    //                      test_that( res1.second.connected(), ==( false ) );
    //                      test_that( res2.second.connected(), ==( true ) );
    //                      test_that( signal.size(), ==( 1 ) );
    //                  } );
    //          } );
    /*
    describe("tracking", [] {
      rsutils::signal<void()> signal;
      before_each([&] { signal = rsutils::signal<void()>{}; });
      it("should disconnect slots when tracked objects are destroyed", [&] {
        struct foo{};
        bool called = false;
        auto tracked = std::make_shared<foo>();
        signal.subscribe([&] {
          called = true;
        }, {tracked});
        tracked.reset();
        signal.raise();
        test_that(called, ==(false));
        signal.compact();
        test_that(signal.size(), ==(0));
      });
    });*/
#if 0
        describe( "connection",
                  []
                  {
                      rsutils::signal< void() > signal;
                      before_each( [&] { signal = rsutils::signal< void() >{}; } );
                      describe( "#connected()",
                                [&]
                                {
                                    it( "should return whether or not the slot is connected",
                                        [&]
                                        {
                                            auto connection = signal.subscribe( [] {} );
                                            test_that( connection.connected(), ==( true ) );
                                            signal.disconnect_all();
                                            test_that( connection.connected(), ==( false ) );
                                        } );
                                } );
                      describe( "#disconnect",
                                [&]
                                {
                                    it( "should disconnect the slot",
                                        [&]
                                        {
                                            bool fired = false;
                                            auto connection = signal.subscribe( [&] { fired = true; } );
                                            connection.disconnect();
                                            signal.raise();
                                            test_that( fired, ==( false ) );
                                            test_that( connection.connected(), ==( false ) );
                                            test_that( signal.size(), ==( 0u ) );
                                        } );
                                    it( "should not throw if already disconnected",
                                        [&]
                                        {
                                            auto connection = signal.subscribe( [] {} );
                                            connection.disconnect();
                                            connection.disconnect();
                                            test_that( connection.connected(), ==( false ) );
                                            test_that( signal.size(), ==( 0u ) );
                                        } );
                                } );
                      it( "should be consistent across copies",
                          [&]
                          {
                              auto conn1 = signal.subscribe( [] {} );
                              auto conn2 = conn1;
                              conn1.disconnect();
                              test_that( conn1.connected(), ==( conn2.connected() ) );
                              test_that( signal.size(), ==( 0u ) );
                          } );
                      it( "should not affect slot lifetime",
                          [&]
                          {
                              bool fired = false;
                              auto fn = [&]
                              {
                                  fired = true;
                              };
                              {
                                  auto connection = signal.subscribe( fn );
                              }
                              signal.raise();
                              test_that( fired, ==( true ) );
                          } );
                      it( "should still be valid if the signal is destroyed",
                          [&]
                          {
                              using connection_type = rsutils::signal< void() >::connection;
                              connection_type connection;
                              {
                                  rsutils::signal< void() > scoped_signal{};
                                  connection = scoped_signal.subscribe( [] {} );
                              }
                              test_that( connection.connected(), ==( false ) );
                          } );
                  } );
        describe( "scoped_connection",
                  []
                  {
                      rsutils::signal< void() > signal;
                      before_each( [&] { signal = rsutils::signal< void() >(); } );
                      it( "should disconnect the connection after leaving the scope",
                          [&]
                          {
                              bool fired = false;
                              {
                                  auto scoped = make_scoped_connection( signal.subscribe( [&] { fired = true; } ) );
                              }
                              signal.raise();
                              test_that( fired, ==( false ) );
                              test_that( signal.empty(), ==( true ) );
                          } );
                      it( "should update state of underlying connection",
                          [&]
                          {
                              auto connection = signal.subscribe( [] {} );
                              {
                                  auto scoped = make_scoped_connection( connection );
                              }
                              signal.raise();
                              test_that( connection.connected(), ==( false ) );
                          } );
                  } );
#endif
    constexpr size_t N = 10000;
    describe( std::to_string( N ),
              [N]
              {
                  rsutils::signal< int & > signal;

                  it( "subscriptions",
                      [&signal, N]
                      {
                          for( int expected = 0; expected < N; ++expected )
                              signal.subscribe(
                                  [expected]( int & i )
                                  {
                                      test_that( i, ==, expected );
                                      ++i;
                                  } );
                          test_that( signal.allocated_size(), ==, N );
                          int i = 0;
                          auto before = std::chrono::high_resolution_clock::now();
                          signal.raise( i );
                          auto after = std::chrono::high_resolution_clock::now();
                          auto delta = after - before;
                          std::cout << "-I-         took " << delta.count() / 1000. << " milliseconds total" << std::endl;
                      } );
                  it( "- 999",
                      [&signal, N]
                      {
                          for( int pos = 0; pos < N-1; ++pos )
                              test_that( signal.unsubscribe( pos ), ==, true );
                          test_that( signal.allocated_size(), ==, N );
                          test_that( signal.size(), ==, 1 );
                          int i = int(N)-1;  // the only one left
                          signal.raise( i );
                      } );
                  it( "+ 1 -> should take a position in the beginning",
                      [&signal, N]
                      {
                          auto slot_id = signal.subscribe( []( int & ) {} );
                          test_that( signal.allocated_size(), ==, N );
                          test_that( slot_id, <, N );
                      } );
              } );
    return ! _failed_signal;
}


}  // namespace

// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.
#pragma once

#pragma warning(push)
// third-party\pybind11\include\pybind11\pybind11.h(232): warning C4702: unreachable code
#if defined(_MSC_VER) && _MSC_VER < 1910  // VS 2015's MSVC
#  pragma warning(disable: 4702)
#endif
#include <pybind11/pybind11.h>
#pragma warning(pop)

// convenience functions
#include <pybind11/operators.h>

// STL conversions
#include <pybind11/stl.h>

// std::chrono::*
#include <pybind11/chrono.h>

// makes certain STL containers opaque to prevent expensive copies
#include <pybind11/stl_bind.h>

// makes std::function conversions work
#include <pybind11/functional.h>

// and enable a bridge to/from rsutils::json
#include "json.h"


namespace py = pybind11;
using namespace pybind11::literals;


// "When calling a C++ function from Python, the GIL is always held"
// -- since we're not being called from Python but instead are calling it, this is different:
// 
// "The wrapper for std::function always acquires the GIL via gil_scoped_acquire when the function is called, so your
// python callback will always be called with the GIL held, regardless which thread it is called from."
//   -- https://stackoverflow.com/questions/72876146/handling-gil-when-calling-python-lambda-from-c-function
//
// So we do not need to explicitly acquire the GIL:
//py::gil_scoped_acquire gil;
// 
// BUT:
// "if nothing else in the thread acquires the thread state and increments the reference count, then once your function
// exits ... it will delete the thread state associated with that thread ... If you're calling the callback often, it
// will create/delete the thread state a lot, which probably isn't great for performance"
// 
// Also, if the callback then calls C++ again (pretty common usage), things get complicated!
//
#define FN_FWD_CALL( CLS, FN_NAME, CODE )                                                                              \
    try                                                                                                                \
    {                                                                                                                  \
        CODE                                                                                                           \
    }                                                                                                                  \
    catch( std::exception const & e )                                                                                  \
    {                                                                                                                  \
        LOG_ERROR( "EXCEPTION in python " #CLS "." #FN_NAME ": " << e.what() );                                        \
    }                                                                                                                  \
    catch( ... )                                                                                                       \
    {                                                                                                                  \
        LOG_ERROR( "UNKNOWN EXCEPTION in python " #CLS "." #FN_NAME ); \
    }
#define FN_FWD( CLS, FN_NAME, PY_ARGS, FN_ARGS, CODE )                                                                 \
    #FN_NAME, []( CLS & self, std::function < void PY_ARGS > callback ) {                                              \
        self.FN_NAME( [&self,callback] FN_ARGS { FN_FWD_CALL( CLS, FN_NAME, CODE ) } );                                \
    }
#define FN_FWD_R( CLS, FN_NAME, RV, PY_ARGS, FN_ARGS, CODE )                                                           \
    FN_FWD_R_( CLS, FN_NAME, decltype(RV), RV, PY_ARGS, FN_ARGS, CODE )
#define FN_FWD_R_( CLS, FN_NAME, RET, RV, PY_ARGS, FN_ARGS, CODE )                                                     \
    #FN_NAME, []( CLS & self, std::function < RET PY_ARGS > callback ) {                                               \
        self.FN_NAME( [&self, callback] FN_ARGS { FN_FWD_CALL( CLS, FN_NAME, CODE ) return RV; } );                    \
    }

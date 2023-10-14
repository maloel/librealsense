// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#pragma once


// Compiler Warning (level 2) C4250: 'class1' : inherits 'class2::member' via dominance
// This happens for librealsense::device, basically warning us of something that we know and is OK:
//     'librealsense::device': inherits 'info_container::create_snapshot' via dominance (and repeats for 3 more functions)
// And for anything else using both an interface and a container, options_container etc.
#pragma warning(disable: 4250)


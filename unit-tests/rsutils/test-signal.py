# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

from rspy import log, test
from pyrsutils import test_signal


with test.closure( "test everything in C++" ):
    test.check( test_signal() )


test.print_results_and_exit()


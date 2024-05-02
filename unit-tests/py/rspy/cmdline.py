# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2024 Intel Corporation. All Rights Reserved.

import sys, os

# We're usually the first to be imported, and so the first see the original arguments as passed
# into sys.argv... remember them before we change:
# (NOTE: sys.orig_argv is available as of 3.10)
original = sys.argv[1:]


def find( flag ):
    """
    Return the index (1 or more) of the flag or None if not found
    """
    for x in range( 1, len(sys.argv) ):
        a = sys.argv[x]
        if a == flag:
            return x
        if a == '--':
            break
    return None


def pop_flag( flag ):
    """
    Remove the flag, if found
    Return True if found
    """
    index = find( flag )
    if index:
        sys.argv.pop( index )
        return True


def pop_arg( flag, default=None ):
    """
    Remove the flag and following value (--flag <value>), if found
    Return the value if found, or the default otherwise
    """
    index = find( flag )
    if index:
        try:
            value = sys.argv.pop( index + 1 )
        except IndexError:
            log.f( f'Missing value for {flag} argument' )
        sys.argv.pop( index )
        return value
    return default


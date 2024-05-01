# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#test:device D400*

import pyrealsense2 as rs
from rspy import test, log
from time import sleep

#rs.log_to_console( rs.log_severity.debug )
rs.log_to_file( rs.log_severity.debug, 'bet.log' )

with test.closure( 'setup', on_fail=test.ABORT ):
    dev = test.find_first_device_or_exit()
    sensor = dev.first_color_sensor()

    # Make sure global time is disabled (keep things as raw as possible)
    sensor.set_option( rs.option.global_time_enabled, 0 )

    profile = next( p for p in sensor.profiles if
            p.fps() == 30
        #and p.format() == rs.format.yuyv
        and p.is_default() )

    def on_frame( frame ):
        # The time-of-arrival is when librealsense sensor gets the frame
        # The backend timestamp is when the backend first sees it
        # I.e., the ToA should always be the same (because we have milli resolution) or after!
        toa = frame.get_frame_metadata( rs.frame_metadata_value.time_of_arrival )
        bet = frame.get_frame_metadata( rs.frame_metadata_value.backend_timestamp )
        test.info( 'time of arrival  ', toa )
        test.info( 'backend timestamp', bet )
        test.check( toa >= bet )
        log.d( f'{frame} {toa-bet}' )

with test.closure( 'run' ):
    sensor.open( profile )
    sensor.start( on_frame )

    sleep( 3 )

    sensor.stop()
    sensor.close()

test.print_results_and_exit()

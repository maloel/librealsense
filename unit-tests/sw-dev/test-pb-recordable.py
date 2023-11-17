# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.


#
# This test checks that recommended processing blocks are recorded and reconstructed on playback
#


import pyrealsense2 as rs
from rspy import log, test
import sw

with test.closure( "Create a software device & color sensor" ):
    device = sw.device()
    sensor = sw.sensor( "RGB Camera", device )
    color = sensor.video_stream( "Color", rs.stream.color, rs.format.yuyv )

with test.closure( "Add a color processing block" ):
    sensor.handle.add_recommended_processing_block( rs.hole_filling_filter() )

with test.closure( "Start recording" ):
    import tempfile, os
    temp_dir = tempfile.TemporaryDirectory( prefix = 'recordings_' )
    filename = os.path.join( temp_dir.name, 'rec.bag' )
    recorder = rs.recorder( filename, device.handle )

with test.closure( "Start streaming" ):
    sensor.start( color )

with test.closure( "Record a frame" ):
    f = color.frame()
    f = sensor.publish( f )

with test.closure( "Stop" ):
    recorder.pause()
    del recorder  # otherwise the file will be open when we exit
    sensor.stop()
    del sensor
    del device

with test.closure( f"Dump the file: {filename}" ):
    # Should look like:
    #     [Depth/0 #0 @0.000000]
    #     [Color/1 #0 @0.000000]
    #
    from rspy import repo
    rs_convert = repo.find_built_exe( 'tools/convert', 'rs-convert' )
    if rs_convert:
        import subprocess
        subprocess.run( [rs_convert, '-i', filename, '-T'],
                        stdout=None,
                        stderr=subprocess.STDOUT,
                        universal_newlines=True,
                        timeout=10,
                        check=False )  # don't fail on errors
    else:
        log.w( 'no rs-convert was found!' )
        import sys
        log.d( 'sys.path=\n    ' + '\n    '.join( sys.path ) )

with test.closure( "Play it back" ):
    ctx = rs.context()
    device = rs.playback( ctx.load_device( filename ) )
    device.set_real_time( False )
    sensors = device.query_sensors()
    test.check_equal( len(sensors), 1, on_fail=test.RAISE )
    sensor = sensors[0]
    test.check_equal( sensor.get_info( rs.camera_info.name ), "RGB Camera" )

with test.closure( "Check processing blocks" ):
    filters = list()
    for filter in sensor.get_recommended_filters():
        filters.append( filter.get_info( rs.camera_info.name ) )
    test.check_equal_lists( filters, list() )

with test.closure( "Close the playback" ):
    del sensor
    del device

#
#############################################################################################
test.print_results_and_exit()

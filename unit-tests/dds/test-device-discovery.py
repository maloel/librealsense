# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#test:donotrun:!dds
#test:retries:gha 2

from rspy import log, test
import pyrealdds as dds

with test.remote.fork( nested_indent='  S' ) as remote:
    if remote is None:  # we're the server fork
        dds.debug( log.is_debug_on(), log.nested )

        participant = dds.participant()
        participant.init( 123, 'server' )

        def create_device_info( props ):
            di = dds.message.device_info()
            di.serial = props.get( 'serial', str( participant.create_guid() ) )
            di.name = props.get( 'name', f'device{di.serial}' )
            di.topic_root = props.get( 'topic_root', f'path/to/{di.name}' )
            return di

        def create_server( root ):
            s1p1 = dds.video_stream_profile( 9, dds.video_encoding.rgb, 10, 10 )
            s1profiles = [s1p1]
            s1 = dds.color_stream_server( 's1', 'sensor' )
            s1.init_profiles( s1profiles, 0 )
            s1.init_options( [
                dds.option( 'Backlight Compensation', dds.option_range( 0, 1, 1, 0 ), 'Backlight custom description' ),
                dds.option( 'Custom Option', dds.option_range( 0, 10, 1, 5 ), 'Description' )
                ] )
            server = dds.device_server( participant, root )
            server.init( [s1], [], {} )
            return server

        # From here down, we're in "interactive" mode (see test-watcher.py)
        # ...
        raise StopIteration()  # quit the remote



    dds.debug( log.is_debug_on(), 'C  ' )
    log.nested = 'C  '

    import threading
    from rspy.stopwatch import Stopwatch


    participant = dds.participant()
    participant.init( 123, "client" )


    # We listen directly on the device-info topic
    device_info_topic = dds.message.device_info.create_topic( participant, dds.topics.device_info )
    device_info = dds.topic_reader( device_info_topic )
    broadcast_received = threading.Event()
    broadcast_devices = []
    def on_device_info_available( reader ):
        while True:
            msg = dds.message.flexible.take_next( reader )
            if not msg:
                break
            j = msg.json_data()
            log.d( f'on_device_info_available {j}' )
            di = dds.message.device_info.from_json( j )
            global broadcast_devices
            broadcast_devices.append( di )
        broadcast_received.set()
    device_info.on_data_available( on_device_info_available )
    device_info.run( dds.topic_reader.qos() )

    def detect_broadcast():
        global broadcast_received, broadcast_devices
        broadcast_received.clear()
        broadcast_devices = []

    def wait_for_broadcast( count=1, timeout=1 ):
        while timeout > 0:
            sw = Stopwatch()
            if not broadcast_received.wait( timeout ):
                raise TimeoutError( 'timeout waiting for broadcast' )
            if count <= len(broadcast_devices):
                return
            broadcast_received.clear()
            timeout -= sw.get_elapsed()
        if count is None:
            raise TimeoutError( 'timeout waiting broadcast' )
        raise TimeoutError( f'timeout waiting for {count} broadcasts; {len(broadcast_devices)} received' )

    class broadcast_expected:
        def __init__( self, n_expected=1, timeout=1 ):
            self._timeout = timeout
            self._n_expected = n_expected
        def __enter__( self ):
            detect_broadcast()
        def __exit__( self, type, value, traceback ):
            if type is None:  # If an exception is thrown, don't do anything
                wait_for_broadcast( count=self._n_expected, timeout=self._timeout )


    # Start a watcher, too...
    change_received = threading.Event()
    devices_added = 0
    devices_removed = 0
    devices = dict()

    def on_device_added( watcher, dev ):
        global devices_added, devices
        devices_added += 1
        log.d( '+++-> device added', dev )
        devices[dev.device_info().topic_root] = dev
        change_received.set()
        test.check( dev.is_online() )

    def on_device_removed( watcher, dev ):
        global devices_removed, devices
        devices_removed += 1
        log.d( '<---- device removed', dev )
        del devices[dev.device_info().topic_root]
        change_received.set()

    def detect_change():
        global devices_added, devices_removed
        change_received.clear()
        devices_added = 0
        devices_removed = 0

    def wait_for_change( n_added=0, n_removed=0, timeout=3 ):
        global devices_added, devices_removed
        while timeout > 0:
            sw = Stopwatch()
            if not change_received.wait( timeout ):
                raise TimeoutError( 'timeout waiting for add/remove' )
            change_received.clear()
            if n_added <= devices_added and n_removed <= devices_removed:
                return
            timeout -= sw.get_elapsed()
        raise TimeoutError( f'timeout waiting for {count} add/removes; {n_changes} received' )

    class change_expected:
        def __init__( self, n_added=0, n_removed=0, timeout=3 ):
            self._timeout = timeout
            self._n_added = n_added
            self._n_removed = n_removed
        def __enter__( self ):
            detect_change()
        def __exit__( self, type, value, traceback ):
            if type is None:  # If an exception is thrown, don't do anything
                wait_for_change( n_added=self._n_added, n_removed=self._n_removed, timeout=self._timeout )
                global devices_added, devices_removed
                test.check_equal( devices_added, self._n_added )
                test.check_equal( devices_removed, self._n_removed )


    watcher = dds.device_watcher( participant )
    watcher.on_device_added( on_device_added )
    watcher.on_device_removed( on_device_removed )
    watcher.start()


    #############################################################################################
    with test.closure( "Broadcast one device" ):
        with change_expected( n_added=1 ):
            remote.run( 'di1 = create_device_info({ "serial" : "123" })' )
            remote.run( 'd1 = create_server( di1.topic_root )' )
            remote.run( 'd1.broadcast( di1 )' )
        test.check_equal( len(broadcast_devices), 1 )
        test.check_equal( len(devices), 1 )
        d1 = devices['path/to/device123']  # remember it -- we'll re-add it later and want to test it's the same!

    #############################################################################################
    with test.closure( "Broadcast second device" ):
        with change_expected( n_added=1 ):
            remote.run( 'di2 = create_device_info({ "serial" : "456" })' )
            remote.run( 'd2 = create_server( di2.topic_root )' )
            remote.run( 'd2.broadcast( di2 )' )
        test.check_equal( len(broadcast_devices), 3 )  # each broadcast is of ALL the devices
        test.check_equal( len(devices), 2 )
        d2guid = devices[f'path/to/device456'].guid()

    #############################################################################################
    with test.closure( "Add another client; expect rebroadcast of all" ):
        with broadcast_expected( 2 ):
            reader_2 = dds.topic_reader( device_info_topic )
            reader_2.run( dds.topic_reader.qos() )
        test.check_equal( len(broadcast_devices), 2 )
        del reader_2

    #############################################################################################
    with test.closure( "We should see both in the watcher" ):
        test.check_equal( len(devices), 2 )
        for dev in devices.values():
            test.info( 'device', dev )
            test.check( watcher.is_device_broadcast( dev ) )

    #############################################################################################
    with test.closure( "Set one option to a non-default value" ):
        option = next( o for o in d1.streams()[0].options() if o.get_name() == 'Custom Option' )
        if test.check( option ):
            test.check_equal( option.get_value(), 5. )
            d1.set_option_value( option, 8. )
            test.check_equal( option.stream().name(), 's1' )

    #############################################################################################
    with test.closure( "Remove both; this should stop the broadcaster thread" ):
        with change_expected( n_removed=2 ):
            remote.run( 'del d1' )
            remote.run( 'del d2' )
        test.check_equal( len(watcher.devices()), 0 )

    #############################################################################################
    with test.closure( "The devices should no longer be broadcasting" ):
        test.check_false( watcher.is_device_broadcast( d1 ) )
        test.check_false( d1.is_online() )

    #############################################################################################
    with test.closure( "Add one back, without a broadcast" ):
        detect_broadcast()
        detect_change()
        remote.run( 'd1 = create_server( di1.topic_root )' )
        test.check_equal( len(broadcast_devices), 0 )
        test.check_equal( devices_added, 0 )

    #############################################################################################
    with test.closure( "It should remain offline (not yet rediscovered)" ):
        test.check_equal( len(devices), 0 )
        test.check_equal( len(watcher.devices()), 0 )
        test.check_false( d1.is_online() )

    #############################################################################################
    with test.closure( "But it should get ready!" ):
        d1.wait_until_ready()
        test.check( d1.is_ready() )
        test.check_false( d1.is_online() )
        test.check_equal( len(devices), 0 )
        test.check_false( watcher.is_device_broadcast( d1 ) )

    #############################################################################################
    with test.closure( "Now broadcast it; it should come online" ):
        with change_expected( n_added=1 ):
            remote.run( 'd1.broadcast( di1 )' )
        test.check_equal( len(devices), 1 )
        test.check( d1.is_online() )
        test.check( watcher.is_device_broadcast( d1 ) )

    #############################################################################################
    with test.closure( "Check that the option value is the new value" ):
        test.check_equal( option.get_value(), 8. )  # cached
        test.check_false( option.stream() )  # no longer valid
        d1.query_option_value( option )
        new_option = next( o for o in d1.streams()[0].options() if o.get_name() == 'Custom Option' )
        test.check_equal( new_option.get_value(), 8. )  # cached, but should get the current value on init...?


    del watcher
    del device_info
    del participant
    test.print_results_and_exit()

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
        participant.init( 123, "server" )
        test.check( participant.is_valid() )

        publisher = dds.publisher( participant )
        broadcasters = []

        def broadcast( props ):
            global broadcasters, publisher
            di = dds.message.device_info()
            di.serial = props.get( 'serial', str(len(broadcasters)) )
            di.name = props.get( 'name', f'device{di.serial}' )
            di.product_line = props.get( 'product_line', '' )
            di.topic_root = props.get( 'topic_root', f'path/to/{di.name}' )
            broadcasters.append( dds.device_broadcaster( publisher, di ) )

        def unbroadcast_all():
            global broadcasters
            broadcasters = []

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
    n_changes = 0
    devices_added = 0
    devices_removed = 0
    devices = dict()

    def on_device_added( watcher, dev ):
        global devices_added, n_changes, devices
        devices_added += 1
        n_changes += 1
        log.d( '+++-> device added', dev )
        devices[dev.device_info().topic_root] = dev
        change_received.set()

    def on_device_removed( watcher, dev ):
        global devices_removed, n_changes, devices
        devices_removed += 1
        n_changes += 1
        log.d( '<---- device removed', dev )
        del devices[dev.device_info().topic_root]
        change_received.set()

    def detect_change():
        global n_changes
        change_received.clear()
        n_changes = 0

    def wait_for_change( count=1, timeout=3 ):
        global n_changes
        while timeout > 0:
            sw = Stopwatch()
            if not change_received.wait( timeout ):
                raise TimeoutError( 'timeout waiting for add/remove' )
            change_received.clear()
            if count <= n_changes:
                return
            timeout -= sw.get_elapsed()
        raise TimeoutError( f'timeout waiting for {count} add/removes; {n_changes} received' )

    class change_expected:
        def __init__( self, n_expected=1, timeout=3 ):
            self._timeout = timeout
            self._n_expected = n_expected
        def __enter__( self ):
            detect_change()
        def __exit__( self, type, value, traceback ):
            if type is None:  # If an exception is thrown, don't do anything
                wait_for_change( count=self._n_expected, timeout=self._timeout )


    watcher = dds.device_watcher( participant )
    watcher.on_device_added( on_device_added )
    watcher.on_device_removed( on_device_removed )
    watcher.start()


    #############################################################################################
    with test.closure( "Broadcast first; expect 1" ):
        with change_expected():
            remote.run( 'broadcast({ "serial" : "123" })' )
        test.check_equal( len(broadcast_devices), 1 )
        test.check_equal( devices_added, 1 )
        test.check_equal( len(devices), 1 )
        for root,dev in devices.items():
            device123 = dev  # remember this device -- we'll re-add it later and want to test it's the same!

    #############################################################################################
    with test.closure( "Broadcast second; expect 1" ):
        with change_expected():
            remote.run( 'broadcast({ "serial" : "456" })' )
        test.check_equal( len(broadcast_devices), 3 )  # each broadcast is of ALL the devices
        test.check_equal( devices_added, 2 )
        test.check_equal( len(devices), 2 )

    #############################################################################################
    with test.closure( "Add another client; expect rebroadcast of all" ):
        with broadcast_expected( 2 ):
            reader_2 = dds.topic_reader( device_info_topic )
            reader_2.run( dds.topic_reader.qos() )
        test.check_equal( len(broadcast_devices), 2 )
        del reader_2

    #############################################################################################
    with test.closure( "We should see both in the watcher" ):
        test.check_equal( devices_added, 2 )
        test.check_equal( len(devices), 2 )

    #############################################################################################
    with test.closure( "Remove both; this should actually stop the broadcaster thread" ):
        test.check_equal( devices_removed, 0 )
        with change_expected( 2 ):
            remote.run( 'unbroadcast_all()' )
        test.check_equal( devices_removed, 2 )
        test.check_equal( len(watcher.devices()), 0 )

    #############################################################################################
    with test.closure( "The devices should no longer be ready" ):
        test.check_false( device123.is_ready() )

    #############################################################################################
    with test.closure( "Add one back" ):
        with change_expected():
            remote.run( 'broadcast({ "serial" : "123" })' )
        test.check_equal( len(devices), 1 )
        test.check_equal( len(watcher.devices()), 1 )

    #############################################################################################
    with test.closure( "Should be same device object as the first one!" ):
        for dev in watcher.devices():
            test.check_equal( dev.guid(), device123.guid() )


    del watcher
    del device_info
    del participant
    test.print_results_and_exit()

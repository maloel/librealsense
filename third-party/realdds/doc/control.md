
# Control

Controls are commands from a client to the server, e.g. start streaming, set option value etc...


## Replies

All control messages need to be re-broadcast with a result, for other clients to be notified and made aware. The [notifications](notifications.md) topic is used for this.

When a control is sent, some way to identify a reply to that specific control must exist:

- Replies will have the same `id` as the original control. This means **controls and notifications must have separate identifiers**.

Additionally, when the server gets a control message, the DDS sample will contain a "publication handle". This is the `GUID` of the writer (including the participant) to identify the control source. A unique sequence number for each control sample is available from the `sample-identity`. Without these, it is impossible to identify the origin of the control and therefore, whether we're waiting on it or not.

- Replies must contain a `sample` field containing the GUID of the participant from which the control request originated, plus the sequence-number, e.g.: `"sample": ["010f6ad8708c95e000000000.303",4]`

Replies must also include a success/failure status:

- Replies will include a `status` for anything other than `OK`
- Replies will include an `explanation` if status is not `OK`

It is recommended that replies also contain the original control request:

- A `control` field in the reply should contain the original control request, though some freedom exists depending on each control

Replies must function even in recovery mode.


### Errors

An error is a reply where the `status` is not OK:

- the `explanation` is a human-readable string explaining the failure (like an exception's `what`)

```JSON
{
    "id": "query-option",
    "sample": ["010f9a5f64d95fd300000000.403", 1],
    "control": {
        "id": "query-option",
        "option-name": "opt16"
        },
    "status": "error",
    "explanation": "Test Device does not support option opt16"
}
```


### Combining Multiple Controls

Unlike [notifications](notifications.md), this is not possible.


## Control Messages

### `query-option` & `set-option`

Returns or sets the current `value` of an option within a device.

- `id` is `query-option` or `set-option`
- `option-name` is the unique option name within its owner
    - Usually this is a single string value, but can also be an array of names for bulk queries: `["option1", "option2", ...]`
- `stream-name` is the unique name of the stream it's in within the device
    - for a device option, this may be omitted
- for `set-option`, `value` is the new value (float)

```JSON
{
    "id": "query-option",
    "option-name": "opt-name"
}
```

The reply should include the original control, plus:

- `value` will include the current or new value
    - In the case of bulk queries, this will be an array of values, in the same size and ordering as the control's `option-name` array: `[<value-of-option1>, <value-of-option2>, ...]`
    - If the `option-name` array is empty, all option values will be returned in a `option-values` key, instead (see below)

```JSON
{
    "id": "query-option",
    "sample": ["010f9a5f64d95fd300000000.403", 1],
    "value": 0.5,
    "control": {
        "id": "query-option",
        "option-name": "opt-name"
        }
}
```

Both querying and setting options involve a very similar reply that can be handled in the same manner.

A new option value should conform to the specific option's value range as communicated when the device was [initialized](initialization.md).

Option values that are changed *by the server* (without a control message) are expected to send `set-option` notifications with no `control` or `sample` fields and an `option-values` array:

```JSON
{
    "id": "set-option",
    "option-values": {
        "option1": 5,
        "option2": "Stacy"
    }
}
```


### `hw-reset`

Can be used to cause the server to perform a "hardware reset", if available, bringing it back to the same state as after power-up.

```JSON
{
    "id": "hw-reset"
}
```

* If boolean `recovery` is set, then the device will reset to recovery mode (error if this is not possible)

A reply can be expected.

A [disconnection event](discovery.md#disconnection) can be expected if the reply is a success.


### `hwm`

Can be used to send internal commands to the hardware and may brick the device if used. May or may not be implemented, and is not documented.

* `data` is required

It is up to the server to validate and make sure everything is as expected.


```JSON
{
    "id": "hwm",
    "data": "1400abcd1000000000000000000000000000000000000000"
}
```

The above is a `GVD` HWM command.

A reply can be expected. Attaching the control is recommended so it's clear what was done. The reply `data` should be preset:

```JSON
{
    "id": "hwm",
    "sample": ["010f9a5f64d95fd300000000.403", 1],
    "control": {
        "id": "hwm",
        "data": "1400abcd1000000000000000000000000000000000000000"
        },
    "data": "10000000011004000e01ffffff01010102000f05000000000000000064006b0000001530ffffffffffffffffffffffffffffffff0365220706600000ffff3935343031300094230500730000ffffffffffffffff0aff3939394146509281a1ffffffffffffffffff9281a1ffffffffffffffffffffffffffffffffffffffffff1f0fffffffffffffffff000027405845ffffffffffffffff908907ffffff01ffffff050aff00260000000200000001000000010000000100000001000000000600010001000100030303020200000000000100070001ffffffffffffffffffffffffffffffffffff014a34323038362d31303001550400ae810100c50004006441050011d00000388401002e0000002dc00000ff"
}
```

#### A more verbose method

You may also instruct the server to **build the HWM** command if you have knowledge of the available opcodes and data needed:

* `opcode` (string) is required
* `param1`, `param2`, `param3`, `param4` are all 32-bit integers whose meaning depends on the `opcode`, and with a default of `0`
* The `data` field, in this case, points to the command data, if needed, and becomes part of the generated HWM

If an optional `build-command` field is present and `true`, then the reply `data` will contain the generated HWM command **without running it**.


#### `hexarray` type

A `hexarray` is a special encoding for a `bytearray`, or array of bytes, in JSON.

Rather than representing as `[0,100,2,255]`, a `hexarray` is instead represented as a string `"006402ff"` which is a hexadecimal representation of all the consecutive bytes, from index 0 onwards.

* Only lower-case alphabetical letters are used (`[0-9a-f]`)
* The length must be even (2 hex digits per byte)
* The first hex pair is for the first byte, the second hex pair for the second, onwards


### DFU

**D**evice **F**irmware **U**pdate is the mechanism with which the device can have its firmware updated.

A device can run the flow when in recovery mode or during normal operation:
* `dfu-start` starts the DFU process
* The client will then publish the binary image over the `dfu` topic
* Once the image is verified, the device will send back a notification, `dfu-ready`
* When ready, a client can then send a `dfu-apply`

#### `dfu-start`

Starts the DFU process.

The device will subscribe to a `dfu` topic under the topic root (accepting a `blob` message, reliable). Enough memory should be allocated to make sure we're ready to receive the image before a reply is sent.

A reply should indicate the image is ready to be received.

#### `dfu-ready`

The device will wait a certain amount of time for an image to be received. It may error out with a timeout.

Only one image can be received, and it should be from the same participant that initiated the DFU.

The image, once received, will be checked for compatibility. If incompatible, incomplete, missing, etc. -- an error will be sent to the client.

Errors can look like this:
```JSON
{
    "id": "dfu-ready",
    "status": "error",
    "explanation": "incompatible FW version (5.17.0.1)"
}
```
If status is not `OK`, then the device is expected to go back to the pre-DFU state and the process needs to start over.

On success, the device may send back a `"crc": <crc32-value>` or `"size": <number-of-bytes>`, to help debug, or any other relevant content:
```JSON
{
    "id": "dfu-ready",
    "crc": <value>
}
```

The `dfu` subscription can be removed at this time.

#### `dfu-apply`

When all the above is successful and the client is ready, a `dfu-apply` is sent to the device.
If for any reason the DFU should be cancelled, the same message is sent but a `"cancel": true` should be present.
In any case, the device needs to reply right away, **before** the DFU is actually applied!

The device then take whatever time is needed to have the image take effect. Progress notifications are recommended and take the same form as the reply:
```JSON
{
    "id": "dfu-apply",
    "progress": 0.2
}
```

At any time, the process may error out. The device is expected to reset itself and the process would have to start over.

On successful application of the image, the device shall perform a hardware-reset and come back up normally.
Before any restart, the device will send a disconnection event on `device-info`.
The disconnection is the signal to the client that the DFU is over.


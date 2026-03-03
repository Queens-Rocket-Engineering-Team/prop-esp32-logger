import uasyncio as asyncio  # type: ignore

import protocol

streamTask = None


def readAllSensors(sensor_list):
    """Read all sensors and return list of (sensor_id, unit_enum, value) tuples."""
    readings = []

    for i, sensor in enumerate(sensor_list):
        value = sensor.takeData()
        if value is None:
            continue
        
        unit_enum = protocol.UNIT_MAP.get(sensor.units, protocol.UNIT_UNITLESS)
        readings.append((i, unit_enum, value))
    return readings


def executeControl(control_list, command_id, command_state):
    """Validate command_id, actuate control, return error code."""
    if command_id < 0 or command_id >= len(control_list):
        return protocol.ERR_INVALID_ID

    control = control_list[command_id]

    if command_state == protocol.CS_OPEN:
        control.open()
    elif command_state == protocol.CS_CLOSED:
        control.close()
    else:
        return protocol.ERR_INVALID_PARAM

    return protocol.ERR_NONE


def executeEstop(control_list):
    """Set all controls to their default states."""
    for control in control_list:
        control.setDefault()


def startStream(sensor_list, sock, frequency_hz, state):
    """Start the async streaming task."""
    global streamTask  # noqa: PLW0603
    stopStream()
    print("Starting stream.")
    streamTask = asyncio.create_task(_streamLoop(sensor_list, sock, frequency_hz, state))


def stopStream():
    """Cancel the streaming task if running."""
    global streamTask  # noqa: PLW0603
    if streamTask and not streamTask.done():
        streamTask.cancel()
        print("Stream stopped.")
    streamTask = None


async def _streamLoop(sensor_list, sock, frequency_hz, state):
    """Async loop: read sensors, build DATA packet, send, sleep."""
    interval = 1.0 / frequency_hz
    try:
        while True:
            readings = readAllSensors(sensor_list)
            data = protocol.make_data(state.next_seq(), state.ts_offset, readings)
            sock.sendall(data)
            await asyncio.sleep(interval)
    except asyncio.CancelledError:
        print("Stream task cancelled.")
    except OSError as e:
        print(f"Stream loop error: {e}")

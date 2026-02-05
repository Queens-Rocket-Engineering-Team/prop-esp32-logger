import socket
import struct

import uasyncio as asyncio  # type: ignore

import protocol


TCP_PORT = 50000

class ConnectionClosedError(Exception):
    """Raised when the remote peer closes the connection."""
    pass


def connectToServer(server_ip, port=TCP_PORT):
    """Create a TCP socket, blocking connect to server, then switch to non-blocking."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.connect((server_ip, port))
    sock.setblocking(False)
    return sock


async def recvExact(sock, n):
    """Read exactly n bytes from a non-blocking socket."""
    buf = b""
    while len(buf) < n:
        try:
            chunk = sock.recv(n - len(buf))
            if not chunk:
                raise ConnectionClosedError("Connection closed by peer")
            buf += chunk
        except OSError:
            await asyncio.sleep(0)
    return buf


async def recvPacket(sock):
    """Read one complete binary packet from the TCP stream.

    Returns (packet_type, sequence, timestamp, payload).
    """
    header = await recvExact(sock, protocol.HEADER_SIZE)
    _version, packet_type, sequence, length, timestamp = struct.unpack(protocol.HEADER_FMT, header)
    payload_len = length - protocol.HEADER_SIZE
    if payload_len > 0:
        payload = await recvExact(sock, payload_len)
    else:
        payload = b""
    return packet_type, sequence, timestamp, payload


def sendPacket(sock, data):
    """Send a complete packet over TCP."""
    sock.sendall(data)

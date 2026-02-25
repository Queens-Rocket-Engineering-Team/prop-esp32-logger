import socket
import struct

import uasyncio as asyncio  # type: ignore


MULTICAST_ADDRESS = "239.255.255.250"
PORT = 1900

def _inet_aton(ip):
    """Convert an IPv4 address from string format to packed binary format."""
    return struct.pack("BBBB", *[int(x) for x in ip.split(".")])

def _createSSDPSocket(multicast_address=MULTICAST_ADDRESS, port=PORT):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.setblocking(False)
    sock.bind(("0.0.0.0", port))

    membershipRequest = struct.pack(
        "4s4s",
        _inet_aton(multicast_address),
        _inet_aton("0.0.0.0"),
    )
    sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, membershipRequest)
    return sock


async def discoverServer():
    """Listen for a valid SSDP M-SEARCH and return the server's IP address.

    One-shot: creates socket, waits for a valid discovery packet,
    extracts server IP from UDP source address, closes socket, returns IP string.
    """
    sock = _createSSDPSocket()
    print("Waiting for server discovery...")

    try:
        while True:
            try:
                data, address = sock.recvfrom(1024)
                message = data.decode("utf-8")

                isValid = all(required in message for required in [
                    'MAN: "ssdp:discover"',
                    "ST: urn:qretprop:espdevice:1",
                    "USER-AGENT: QRET/1.0",
                ])

                if isValid:
                    server_ip = address[0]
                    print(f"Discovered server at {server_ip}")
                    return server_ip

            except OSError:
                await asyncio.sleep(0.1)
    finally:
        sock.close()

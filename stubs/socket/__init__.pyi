"""TCP, UDP and RAW socket support

.. warning:: This module is disabled in 6.x and will removed in 7.x. Please use networking
             libraries instead. (Native networking will provide a socket compatible class.)

Create TCP, UDP and RAW sockets for communicating over the Internet."""

from __future__ import annotations

from typing import Optional, Tuple

from _typing import ReadableBuffer, WriteableBuffer

class socket:

    AF_INET: int
    AF_INET6: int
    SOCK_STREAM: int
    SOCK_DGRAM: int
    SOCK_RAW: int
    IPPROTO_TCP: int
    def __init__(
        self, family: int = AF_INET, type: int = SOCK_STREAM, proto: int = IPPROTO_TCP
    ) -> None:
        """Create a new socket

        :param int family: AF_INET or AF_INET6
        :param int type: SOCK_STREAM, SOCK_DGRAM or SOCK_RAW
        :param int proto: IPPROTO_TCP, IPPROTO_UDP or IPPROTO_RAW (ignored)"""
        ...
    def bind(self, address: Tuple[str, int]) -> None:
        """Bind a socket to an address

        :param address: tuple of (remote_address, remote_port)
        :type address: tuple(str, int)"""
        ...
    def listen(self, backlog: int) -> None:
        """Set socket to listen for incoming connections

        :param int backlog: length of backlog queue for waiting connetions"""
        ...
    def accept(self) -> Tuple[socket, str]:
        """Accept a connection on a listening socket of type SOCK_STREAM,
        creating a new socket of type SOCK_STREAM.
        Returns a tuple of (new_socket, remote_address)"""
    def connect(self, address: Tuple[str, int]) -> None:
        """Connect a socket to a remote address

        :param address: tuple of (remote_address, remote_port)
        :type address: tuple(str, int)"""
        ...
    def send(self, bytes: ReadableBuffer) -> int:
        """Send some bytes to the connected remote address.
        Suits sockets of type SOCK_STREAM

        :param ~_typing.ReadableBuffer bytes: some bytes to send"""
        ...
    def recv_into(self, buffer: WriteableBuffer, bufsize: int) -> int:
        """Reads some bytes from the connected remote address, writing
        into the provided buffer. If bufsize <= len(buffer) is given,
        a maximum of bufsize bytes will be read into the buffer. If no
        valid value is given for bufsize, the default is the length of
        the given buffer.

        Suits sockets of type SOCK_STREAM
        Returns an int of number of bytes read.

        :param ~_typing.WriteableBuffer buffer: buffer to receive into
        :param int bufsize: optionally, a maximum number of bytes to read."""
        ...
    def recv(self, bufsize: int) -> bytes:
        """Reads some bytes from the connected remote address.
        Suits sockets of type SOCK_STREAM
        Returns a bytes() of length <= bufsize

        :param int bufsize: maximum number of bytes to receive"""
        ...
    def sendto(self, bytes: ReadableBuffer, address: Tuple[str, int]) -> int:
        """Send some bytes to a specific address.
        Suits sockets of type SOCK_DGRAM

        :param ~_typing.ReadableBuffer bytes: some bytes to send
        :param address: tuple of (remote_address, remote_port)
        :type address: tuple(str, int)"""
        ...
    def recvfrom(self, bufsize: int) -> Tuple[bytes, Tuple[str, int]]:
        """Reads some bytes from the connected remote address.
        Suits sockets of type SOCK_STREAM

        Returns a tuple containing
        * a bytes() of length <= bufsize
        * a remote_address, which is a tuple of ip address and port number

        :param int bufsize: maximum number of bytes to receive"""
        ...
    def setsockopt(self, level: int, optname: int, value: int) -> None:
        """Sets socket options"""
        ...
    def settimeout(self, value: int) -> None:
        """Set the timeout value for this socket.

        :param int value: timeout in seconds.  0 means non-blocking.  None means block indefinitely."""
        ...
    def setblocking(self, flag: bool) -> Optional[int]:
        """Set the blocking behaviour of this socket.

        :param bool flag: False means non-blocking, True means block indefinitely."""
        ...

def getaddrinfo(host: str, port: int) -> Tuple[int, int, int, str, str]:
    """Gets the address information for a hostname and port

    Returns the appropriate family, socket type, socket protocol and
    address information to call socket.socket() and socket.connect() with,
    as a tuple."""
    ...

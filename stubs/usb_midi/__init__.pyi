"""MIDI over USB

The `usb_midi` module contains classes to transmit and receive MIDI messages over USB."""

from __future__ import annotations

from typing import Optional, Tuple, Union

from _typing import ReadableBuffer, WriteableBuffer

ports: Tuple[Union[PortIn, PortOut], ...]
"""Tuple of all MIDI ports. Each item is ether `PortIn` or `PortOut`."""

class PortIn:
    """Receives midi commands over USB"""

    def __init__(self) -> None:
        """You cannot create an instance of `usb_midi.PortIn`.

        PortIn objects are constructed for every corresponding entry in the USB
        descriptor and added to the ``usb_midi.ports`` tuple."""
        ...
    def read(self, nbytes: Optional[int] = None) -> Optional[bytes]:
        """Read characters.  If ``nbytes`` is specified then read at most that many
        bytes. Otherwise, read everything that arrives until the connection
        times out. Providing the number of bytes expected is highly recommended
        because it will be faster.

        :return: Data read
        :rtype: bytes or None"""
        ...
    def readinto(
        self, buf: WriteableBuffer, nbytes: Optional[int] = None
    ) -> Optional[bytes]:
        """Read bytes into the ``buf``.  If ``nbytes`` is specified then read at most
        that many bytes.  Otherwise, read at most ``len(buf)`` bytes.

        :return: number of bytes read and stored into ``buf``
        :rtype: bytes or None"""
        ...

class PortOut:
    """Sends midi messages to a computer over USB"""

    def __init__(self) -> None:
        """You cannot create an instance of `usb_midi.PortOut`.

        PortOut objects are constructed for every corresponding entry in the USB
        descriptor and added to the ``usb_midi.ports`` tuple."""
    def write(self, buf: ReadableBuffer) -> Optional[int]:
        """Write the buffer of bytes to the bus.

        :return: the number of bytes written
        :rtype: int or None"""
        ...

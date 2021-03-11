"""USB Human Interface Device

The `usb_hid` module allows you to output data as a HID device."""

from __future__ import annotations

from typing import Tuple

from _typing import ReadableBuffer

devices: Tuple[Device, ...]
"""Tuple of all active HID device interfaces."""

class Device:
    """HID Device

    Usage::

       import usb_hid

       mouse = usb_hid.devices[0]

       mouse.send_report()"""

    def __init__(self) -> None:
        """Not currently dynamically supported."""
        ...
    def send_report(self, buf: ReadableBuffer) -> None:
        """Send a HID report."""
        ...
    last_received_report: bytes
    """The HID OUT report as a `bytes`. (read-only). `None` if nothing received."""

    usage_page: int
    """The usage page of the device as an `int`. Can be thought of a category. (read-only)"""

    usage: int
    """The functionality of the device as an int. (read-only)

    For example, Keyboard is 0x06 within the generic desktop usage page 0x01.
    Mouse is 0x02 within the same usage page."""

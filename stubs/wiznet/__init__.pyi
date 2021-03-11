"""Support for WizNet hardware, including the WizNet 5500 Ethernet adaptor.


.. warning:: This module is disabled in 6.x and will removed in 7.x. Please use networking
             libraries instead.
"""

from __future__ import annotations

from typing import Optional, Tuple

import busio
import microcontroller

class WIZNET5K:
    """Wrapper for Wiznet 5500 Ethernet interface"""

    def __init__(
        self,
        spi: busio.SPI,
        cs: microcontroller.Pin,
        rst: microcontroller.Pin,
        dhcp: bool = True,
    ) -> None:
        """Create a new WIZNET5500 interface using the specified pins

        :param ~busio.SPI spi: spi bus to use
        :param ~microcontroller.Pin cs: pin to use for Chip Select
        :param ~microcontroller.Pin rst: pin to use for Reset (optional)
        :param bool dhcp: boolean flag, whether to start DHCP automatically (optional, keyword only, default True)

        * The reset pin is optional: if supplied it is used to reset the
          wiznet board before initialization.
        * The SPI bus will be initialized appropriately by this library.
        * At present, the WIZNET5K object is a singleton, so only one WizNet
          interface is supported at a time."""
        ...
    connected: bool
    """(boolean, readonly) is this device physically connected?"""

    dhcp: bool
    """(boolean, readwrite) is DHCP active on this device?

    * set to True to activate DHCP, False to turn it off"""
    def ifconfig(
        self, params: Optional[Tuple[str, str, str, str]] = None
    ) -> Optional[Tuple[str, str, str, str]]:
        """Called without parameters, returns a tuple of
        (ip_address, subnet_mask, gateway_address, dns_server)

        Or can be called with the same tuple to set those parameters.
        Setting ifconfig parameters turns DHCP off, if it was on."""
        ...

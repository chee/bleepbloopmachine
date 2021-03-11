"""Network Interface Management

.. warning:: This module is disabled in 6.x and will removed in 7.x. Please use networking
             libraries instead.

This module provides a registry of configured NICs.
It is used by the 'socket' module to look up a suitable
NIC when a socket is created."""

from __future__ import annotations

from typing import List

def route() -> List[object]:
    """Returns a list of all configured NICs."""
    ...

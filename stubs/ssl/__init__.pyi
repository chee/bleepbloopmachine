"""
The `ssl` module provides SSL contexts to wrap sockets in.
"""

from __future__ import annotations

import ssl
from typing import Optional

import socketpool

def create_default_context() -> ssl.SSLContext:
    """Return the default SSLContext."""
    ...

class SSLContext:
    """Settings related to SSL that can be applied to a socket by wrapping it.
    This is useful to provide SSL certificates to specific connections
    rather than all of them."""

def wrap_socket(
    sock: socketpool.Socket,
    *,
    server_side: bool = False,
    server_hostname: Optional[str] = None
) -> socketpool.Socket:
    """Wraps the socket into a socket-compatible class that handles SSL negotiation.
    The socket must be of type SOCK_STREAM."""
    ...

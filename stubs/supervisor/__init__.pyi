"""Supervisor settings"""

from __future__ import annotations

runtime: Runtime
"""Runtime information, such as ``runtime.serial_connected``
(USB serial connection status).
This object is the sole instance of `supervisor.Runtime`."""

def enable_autoreload() -> None:
    """Enable autoreload based on USB file write activity."""
    ...

def disable_autoreload() -> None:
    """Disable autoreload based on USB file write activity until
    `enable_autoreload` is called."""
    ...

def set_rgb_status_brightness(brightness: int) -> None:
    """Set brightness of status neopixel from 0-255
    `set_rgb_status_brightness` is called."""
    ...

def reload() -> None:
    """Reload the main Python code and run it (equivalent to hitting Ctrl-D at the REPL)."""
    ...

def set_next_stack_limit(size: int) -> None:
    """Set the size of the stack for the next vm run. If its too large, the default will be used."""
    ...

class RunReason:
    """The reason that CircuitPython started running."""

    STARTUP: object
    """CircuitPython started the microcontroller started up. See `microcontroller.Processor.reset_reason`
       for more detail on why the microcontroller was started."""

    AUTO_RELOAD: object
    """CircuitPython restarted due to an external write to the filesystem."""

    SUPERVISOR_RELOAD: object
    """CircuitPython restarted due to a call to `supervisor.reload()`."""

    REPL_RELOAD: object
    """CircuitPython started due to the user typing CTRL-D in the REPL."""

class Runtime:
    """Current status of runtime objects.

    Usage::

       import supervisor
       if supervisor.runtime.serial_connected:
           print("Hello World!")"""

    def __init__(self) -> None:
        """You cannot create an instance of `supervisor.Runtime`.
        Use `supervisor.runtime` to access the sole instance available."""
        ...
    serial_connected: bool
    """Returns the USB serial communication status (read-only)."""

    serial_bytes_available: int
    """Returns the whether any bytes are available to read
    on the USB serial input.  Allows for polling to see whether
    to call the built-in input() or wait. (read-only)"""

    run_reason: RunReason
    """Returns why CircuitPython started running this particular time."""

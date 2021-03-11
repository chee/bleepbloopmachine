"""Comparison functions"""

from __future__ import annotations

from typing import List, Union

import ulab

def clip(
    x1: Union[ulab.array, float],
    x2: Union[ulab.array, float],
    x3: Union[ulab.array, float],
) -> ulab.array:
    """
    Constrain the values from ``x1`` to be between ``x2`` and ``x3``.
    ``x2`` is assumed to be less than or equal to ``x3``.

    Arguments may be ulab arrays or numbers.  All array arguments
    must be the same size.  If the inputs are all scalars, a 1-element
    array is returned.

    Shorthand for ``ulab.maximum(x2, ulab.minimum(x1, x3))``"""
    ...

def equal(x1: Union[ulab.array, float], x2: Union[ulab.array, float]) -> List[bool]:
    """Return an array of bool which is true where x1[i] == x2[i] and false elsewhere"""
    ...

def not_equal(x1: Union[ulab.array, float], x2: Union[ulab.array, float]) -> List[bool]:
    """Return an array of bool which is false where x1[i] == x2[i] and true elsewhere"""
    ...

def maximum(x1: Union[ulab.array, float], x2: Union[ulab.array, float]) -> ulab.array:
    """
    Compute the element by element maximum of the arguments.

    Arguments may be ulab arrays or numbers.  All array arguments
    must be the same size.  If the inputs are both scalars, a number is
    returned"""
    ...

def minimum(x1: Union[ulab.array, float], x2: Union[ulab.array, float]) -> ulab.array:
    """Compute the element by element minimum of the arguments.

    Arguments may be ulab arrays or numbers.  All array arguments
    must be the same size.  If the inputs are both scalars, a number is
    returned"""
    ...

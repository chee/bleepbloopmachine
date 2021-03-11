"""Linear algebra functions"""

from __future__ import annotations

from typing import Optional, Tuple, Union

import ulab

def cholesky(A: ulab.array) -> ulab.array:
    """
    :param ~ulab.array A: a positive definite, symmetric square matrix
    :return ~ulab.array L: a square root matrix in the lower triangular form
    :raises ValueError: If the input does not fulfill the necessary conditions

    The returned matrix satisfies the equation m=LL*"""
    ...

def det(m: ulab.array) -> float:
    """
    :param: m, a square matrix
    :return float: The determinant of the matrix

    Computes the eigenvalues and eigenvectors of a square matrix"""
    ...

def dot(m1: ulab.array, m2: ulab.array) -> Union[ulab.array, float]:
    """
    :param ~ulab.array m1: a matrix, or a vector
    :param ~ulab.array m2: a matrix, or a vector

    Computes the product of two matrices, or two vectors. In the letter case, the inner product is returned."""
    ...

def eig(m: ulab.array) -> Tuple[ulab.array, ulab.array]:
    """
    :param m: a square matrix
    :return tuple (eigenvectors, eigenvalues):

    Computes the eigenvalues and eigenvectors of a square matrix"""
    ...

def inv(m: ulab.array) -> ulab.array:
    """
    :param ~ulab.array m: a square matrix
    :return: The inverse of the matrix, if it exists
    :raises ValueError: if the matrix is not invertible

    Computes the inverse of a square matrix"""
    ...

def norm(x: ulab.array) -> float:
    """
    :param ~ulab.array x: a vector or a matrix

    Computes the 2-norm of a vector or a matrix, i.e., ``sqrt(sum(x*x))``, however, without the RAM overhead."""
    ...

def size(array: ulab.array, *, axis: Optional[int] = None) -> int:
    """Return the total number of elements in the array, as an integer."""
    ...

def trace(m: ulab.array) -> float:
    """
    :param m: a square matrix

    Compute the trace of the matrix, the sum of its diagonal elements."""
    ...

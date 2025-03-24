

from typing import Sized


def assert_equal_lengths(
    *args: Sized, msg: str = "iterable arguments must have same length."
) -> None:
    lengths = set()
    for item in args:
        lengths.add(len(item))
    if len(lengths) != 1:
        raise ValueError(msg)

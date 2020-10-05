# TODO

This document describes features to be incorporated in the future.

## HIGH

- `jscon_composite()`
  - Inner function `jscon_htwrap_link_preorder()` is uneccessarily linking htwraps that are already linked, this does nothing, but decreases performance. Instead of doing it recursively I could try to do it iteratively instead, which would grant higher control of conditional breaks, but is harder to implement.
- Organize APIReference.md in a more intuitive manner.
- Makefile should allow for different keywords to activate each test

## MEDIUM

- Replace memory allocation asserts to a simple return NULL so that the user may decide himself how to deal with the error.
- Add Unicode support
- Add more example codes
- Add stringify formatting options

## LOW

- Create a function that fetches all items with a given key, and then returns a `jscon_item_st` root containing references (branches) to every value found with matching keys.
  - The following json string: `{"obj1": {"n": 1}, "obj2": {"n": 3}, "obj3: {"n": 5}}` after giving "n" key as parameter would then be reorganized as such: `{"obj1": 1, "obj2": 3, "obj3": 5}`, whereas "n" becomes the root key (which is ommited), and the parent becomes the new key identifier to each matching result (to guarantee uniqueness among keys).

  


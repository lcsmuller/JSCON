# JSCON TODO

This document describes features to be incorporated in the future.

## HIGH

- Organize APIReference.md in a more intuitive manner.
- Add Unicode support

## MEDIUM

- Activate debug mode with Makefile
- Turn `jscon_scanf()` into a `jscon_vscanf()` wrapper
- Create a `jscon_printf()` functions following `jscon_scanf()` format rules
- Create a jscon function that open a json text file and converts it to a string automatically.
- Add more example codes
- Add stringify formatting options

## LOW

- Create a function that fetches all items with a given key, and then returns a `jscon_item_st` root containing references (branches) to every value found with matching keys.
  - The following json string: `{"obj1": {"n": 1}, "obj2": {"n": 3}, "obj3: {"n": 5}}` after giving "n" key as parameter would then be reorganized as such: `{"obj1": 1, "obj2": 3, "obj3": 5}`, whereas "n" becomes the root key (which is ommited), and the parent becomes the new key identifier to each matching result (to guarantee uniqueness among keys).

  


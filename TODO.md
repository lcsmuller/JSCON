# TODO

This document describes features to be incorporated in the future.

## HIGH

- `jscon_attach()`
  - Doing approximately 30% more mallocs than its `jscon_list_append()` counterpart, one of the reasons is because of realloc, which is expected, but the other reason is because its remaking the hashtable for every new branch attached, which is uneccessary. This should be taken care of.
- `jscon_composite()`
  - Inner function `jscon_htwrap_link_preorder()` is uneccessarily linking htwraps that are already linked, this does nothing, but decreases performance. Instead of doing it recursively I could try to do it iteratively instead, which would grant higher control of conditional breaks, but is harder to implement.
- Organize APIReference.md in a more intuitive manner.
- Add a hashtable bucket size check when doing functions that increase the total number of branches.
  - If number of branches is equal to hashtable bucket size, then remake hashtable with current branch size in consideration.

- Setter Functions
  - Add singletons `jscon_item_st` initializers.
    - For dynamically creating `jscon_item_st` a jscon entity without depending solely on parsing a json string via `jscon_parse()`.
  - Add item appending function.
    - For dynamically creating a `jscon_item_st` tree that can be later encoded via `jscon_stringify()`
  - Makefile should allow for different keywords to activate each test

## MEDIUM

- Replace memory allocation asserts to a simple return NULL so that the user may decide himself how to deal with the error.
- Add Unicode support
- Add example codes
- Add stringify formatting options

## LOW

- Create a function that fetches all items with a given key, and then returns a `jscon_item_st` root containing references (branches) to every value found with matching keys.
  - The following json string: `{"obj1": {"n": 1}, "obj2": {"n": 3}, "obj3: {"n": 5}}` after giving "n" key as parameter would then be reorganized as such: `{"obj1": 1, "obj2": 3, "obj3": 5}`, whereas "n" becomes the root key (which is ommited), and the parent becomes the new key identifier to each matching result (to guarantee uniqueness among keys).

  


# TODO

This document describes features to be incorporated in the future.

## HIGH

- Add a hashtable bucket size check when doing functions that increase the total number of branches.
  - If number of branches is equal to hashtable bucket size, then remake hashtable with current branch size in consideration.

- Setter Functions
  - Add singletons `jscon_item_st` initializers.
    - For dynamically creating `jscon_item_st` a jscon entity without depending solely on parsing a json string via `jscon_parse()`.
  - Add item appending function.
    - For dynamically creating a `jscon_item_st` tree that can be later encoded via `jscon_stringify()`

## MEDIUM

- Add Unicode support

## LOW

- Create a function that fetches all items with a given key, and then returns a `jscon_item_st` root containing references (branches) to every value found with matching keys.
  - The following json string: [`{"obj1": {"n": 1}, "obj2": {"n": 3}, "obj3: {"n": 5}}`] after giving "n" key as parameter would then be reorganized as such: [`{"obj1": 1, "obj2": 3, "obj3": 5}`], whereas "n" becomes the root key (which is ommited), and the parent becomes the new key identifier to each matching result (to guarantee uniqueness among keys).

  


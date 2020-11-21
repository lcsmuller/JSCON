# JSCON API Reference

### `jscon_item_t;`

### Fields

| Field | Type | Description |
| :--- | :--- | :--- |
|**`key`**|`char *`| The key string of this item |
|**`parent`**|`jscon_item_t *`| The parent of this item |
|**`type`**|[`enum jscon_type`](jscon_type.md)| The datatype of this item |
|**`union {string, d_number, i_number, boolean, comp}`**|`union`| The datatypes this item may activate based on its type |

These fields should **NOT** be written to directly, use the library public functions for that purpose.

### Description

The structure `jscon_item_t` is created by decoding JSON data when calling [`jscon_parse()`](jscon_parse.md). It can be manipulated via functions, such as [`jscon_next()`](jscon_next.md) or [`jscon_get_branch()`](jscon_get_branch.md), and encoded back to JSON data by [`jscon_stringify()`](jscon_stringify.md). This structure **MUST** have a corresponding call to [`jscon_destroy()`](jscon_destroy.md) once its no longer needed.

### See Also

* [`enum jscon_type;`](jscon_type.md)
* [`jscon_parse(buffer);`](jscon_parse.md)
* [`jscon_next(item);`](jscon_next.md)
* [`jscon_get_branch(item, key);`](jscon_get_branch.md)
* [`jscon_stringify(item, type);`](jscon_stringify.md)
* [`jscon_destroy(item);`](jscon_destroy.md)

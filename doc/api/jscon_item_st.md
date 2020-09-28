# JSCON API Reference

### `jscon_item_st;`

### Fields

| Field | Type | Description |
| :--- | :--- | :--- |
|**`key`**|`char *`| The key string of this item |
|**`parent`**|`jscon_item_st *`| The parent of this item |
|**`type`**|[`jscon_type_et`](jscon_type_et.md)| The datatype of this item |
|**`union {string, d_number, i_number, boolean, comp}`**|`union`| The datatypes this item may activate based on its type |

These fields should **NOT** be written to directly, use the library public functions for that purpose.

### Description

The structure `jscon_item_st` is created by decoding JSON data when calling [`jscon_parse(buffer);`](jscon_parse.md). It can be manipulated via functions, such as [`jscon_next(item);`](jscon_next.md) or [`jscon_get_branch(item, key);`](jscon_get_branch.md), and encoded back to JSON data by [`jscon_stringify(item, type);`](jscon_stringify.md). This structure **MUST** have a corresponding call to [`jscon_destroy(item);`](jscon_destroy.md) once its no longer needed.

### See Also

* [`jscon_type_et;`](jscon_type_et.md)
* [`jscon_parse(buffer);`](jscon_parse.md)
* [`jscon_next(item);`](jscon_next.md)
* [`jscon_get_branch(item, key);`](jscon_get_branch.md)
* [`jscon_stringify(item, type);`](jscon_stringify.md)
* [`jscon_destroy(item);`](jscon_destroy.md)

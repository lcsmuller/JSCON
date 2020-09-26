# JSONc API Reference

### `jsonc_item_st;`

### Fields

| Field | Type | Description |
| :--- | :--- | :--- |
|**`key`**|`char *`| The string key of this item |
|**`parent`**|`jsonc_item_st *`| The parent of this item |
|**`type`**|`jsonc_type_et`| The datatype of this item |
|**`union {string, d_number, i_number, boolean, comp}`**|`union`| The datatypes this item can activate based on its type |

### Description

The structure `jsonc_item_st` is created by decoding JSON data when calling [`jsonc_parse(buffer);`](jsonc_parse.md). It can be manipulated via functions, such as [`jsonc_next(item);`](jsonc_next.md) or [`jsonc_get_branch(item, key);`](jsonc_get_branch.md), and encoded back to JSON data via [`jsonc_stringify(item, type);`](jsonc_stringify.md).

### See Also

* [`jsonc_parse(buffer);`](jsonc_parse.md)
* [`jsonc_next(item);`](jsonc_next.md)
* [`jsonc_get_branch(item, key);`](jsonc_get_branch.md)
* [`jsonc_stringify(item, type);`](jsonc_stringify.md)

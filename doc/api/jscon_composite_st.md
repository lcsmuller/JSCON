# JSCON API Reference

### `jscon_composite_st;`

### Fields

| Field | Type | Description |
| :--- | :--- | :--- |
|**`branch`**|`jscon_item_st **`| The properties/elements of the object/array |
|**`num_branch`**|`size_t`| The amount of properties/elements the object/array contains |
|**`last_accessed_branch`**|`size_t`| The last branch accessed from this object/array, see [`jscon_next(item);`](jscon_next.md)) |
|**`htwrap`**|`jscon_htwrap_st`| The hashtable the object/array contains |

### Description

The structure `jscon_composite_st` is the active value in the structure [`jscon_item_st`](jscon_item_st.md) of `JSCON_OBJECT` and `JSCON_ARRAY`
[`type`](jscon_type_et.md).

### See Also

* [`jscon_item_st;`](jscon_item_st.md)
* [`jscon_type_et;`](jscon_type_et.md)
* [`jscon_next(item);`](jscon_next.md)

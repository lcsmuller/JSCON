# JSONc API Reference

### `jsonc_composite_st;`

### Fields

| Field | Type | Description |
| :--- | :--- | :--- |
|**`branch`**|`jsonc_item_st **`| The properties/elements of the object/array |
|**`num_branch`**|`size_t`| The amount of properties/elements the object/array contains |
|**`last_accessed_branch`**|`size_t`| The last branch accessed from this object/array, for simulating tree traversal |
|**`htwrap`**|`jsonc_htwrap_st`| The hashtable the object/array contains |

### Description

The structure `jsonc_composite_st` is the active value in the [`jsonc_item_st`](jsonc_item_st.md) structure when its of an object or array datatype.

### See Also

* [`jsonc_item_st`](jsonc_item_st.md)

# JSCON API Reference

### `jscon_parse(buffer);`

### Parameters

| Parameter | Type | Description |
| :--- | :--- | :--- |
|**`buffer`**|`char *`| The JSON string to be parsed |

### Return Value

| Type | Description |
| :--- | :--- |
|[`jscon_item_st *`](jscon_item_st.md)| A pointer to the root item |

### Description

The function `jscon_parse(buffer);` returns the [`jscon_item_st;`](jscon_item_st.md) root element obtained from the parsed JSON string. This call **MUST** have a corresponding call to [`jscon_destroy(item);`](jscon_destroy.md).

### See Also

* [`jscon_item(buffer);`](jscon_item.md)
* [`jscon_stringify(item, type);`](jscon_stringify.md)
* [`jscon_next(item);`](jscon_next.md)
* [`jscon_get_branch(item, key);`](jscon_get_branch.md)
* [`jscon_destroy(item);`](jscon_destroy.md)

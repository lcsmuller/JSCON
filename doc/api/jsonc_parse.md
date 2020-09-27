# JSONc API Reference

### `jsonc_parse(buffer);`

### Parameters

| Parameter | Type | Description |
| :--- | :--- | :--- |
|**`buffer`**|`char *`| The JSON string to be parsed |

### Return Value

| Type | Description |
| :--- | :--- |
|`jsonc_item_st *`| A pointer to the root item |

### Description

The function `jsonc_parse(buffer);` returns the [`jsonc_item_st;`](jsonc_item_st.md) root obtained from parsing the JSON string. 

### See Also

* [`jsonc_item(buffer);`](jsonc_item.md)
* [`jsonc_stringify(item, type);`](jsonc_stringify.md)
* [`jsonc_next(item);`](jsonc_next.md)
* [`jsonc_get_branch(item, key);`](jsonc_get_branch.md)
* [`jsonc_destroy(item);`](jsonc_destroy.md)

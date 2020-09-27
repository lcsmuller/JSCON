# JSONc API Reference

### `jsonc_parser_callback(new_cb);`

### Parameters

| Parameter | Type | Description |
| :--- | :--- | :--- |
|**`new_cb`**|`jsonc_callbacks_ft *`| The callback function pointer |

### Return Value

| Type | Description |
| :--- | :--- |
|`jsonc_callbacks_ft *`| The custom callback address, NULL if not defined |

### Description

The function `jsonc_parser_callback(new_cb);` is called everytime a item is created inside [`jsonc_parse(buffer);`](jsonc_parse.md), for more information read [`jsonc_callbacks_ft;`](jsonc_callbacks_ft.md).

### See Also

* [`jsonc_parse(buffer);`](jsonc_parse.md)
* [`jsonc_callbacks_ft;`](jsonc_callbacks_ft.md)

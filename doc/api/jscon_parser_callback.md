# JSCON API Reference

### `jscon_parser_callback(new_cb);`

### Parameters

| Parameter | Type | Description |
| :--- | :--- | :--- |
|**`new_cb`**|`jscon_callbacks_ft *`| The callback function pointer |

### Return Value

| Type | Description |
| :--- | :--- |
|`jscon_callbacks_ft *`| The custom callback pointer. If not defined will return NULL and a default callback will be used instead |

### Description

The function `jscon_parser_callback()` is called everytime a new item is created inside [`jscon_parse()`](jscon_parse.md), for more information read [`jscon_callbacks_ft`](jscon_callbacks_ft.md).

### See Also

* [`jscon_parse(buffer);`](jscon_parse.md)
* [`jscon_callbacks_ft;`](jscon_callbacks_ft.md)

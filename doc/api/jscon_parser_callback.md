# JSCON API Reference

### `jscon_parser_callback(new_cb);`

### Parameters

| Parameter | Type | Description |
| :--- | :--- | :--- |
|**`new_cb`**|`jscon_callbacks_ft *`| The callback function pointer |

### Return Value

| Type | Description |
| :--- | :--- |
|`jscon_callbacks_ft *`| The custom callback pointer, will be set to NULL if not defined |

### Description

The function `jscon_parser_callback(new_cb);` is called everytime a item is created inside [`jscon_parse(buffer);`](jscon_parse.md), for more information read [`jscon_callbacks_ft;`](jscon_callbacks_ft.md).

### See Also

* [`jscon_parse(buffer);`](jscon_parse.md)
* [`jscon_callbacks_ft;`](jscon_callbacks_ft.md)

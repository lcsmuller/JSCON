# JSCON API Reference

### `jscon_parser_callback(new_cb);`

### Parameters

| Parameter | Type | Description |
| :--- | :--- | :--- |
|**`new_cb`**|`jscon_cb *`| The callback function pointer |

### Return Value

| Type | Description |
| :--- | :--- |
|`jscon_cb *`| The custom callback pointer. If not defined will return NULL and a default callback will be used instead |

### Description

The function `jscon_parser_callback()` is called everytime a new item is created inside [`jscon_parse()`](jscon_parse.md), for more information read [`jscon_cb`](jscon_cb.md).

### See Also

* [`jscon_parse(buffer);`](jscon_parse.md)
* [`jscon_cb;`](jscon_cb.md)

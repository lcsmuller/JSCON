# JSCON API Reference

### `jscon_destroy(item);`

### Parameters

| Parameter | Type | Description |
| :--- | :--- | :--- |
|**`item`**|[`jscon_item_t *`](jscon_item_t.md)| The JSCON item to be destroyed along with its nests |

### Description

The `jscon_destroy()` is the cleanup procedure that must be called for every corresponding JSCON item initialized with a decoding function, such as [`jscon_parse()`](jscon_parse.md) or [`jscon_scanf()`](jscon_scanf.md). The item is destroyed recursively along with its nests but higher hierarchy items are ignored, the item given as parameter has to be root in order for the initialized [`jscon_item_t`](jscon_item_t.md) be destroyed entirely.

### See Also

* [`jscon_get_root(item);`](jscon_get_root.md)
* [`jscon_parse(item);`](jscon_parse.md)
* [`jscon_item_t;`](jscon_item_t.md)
* [`jscon_scanf(buffer, format, ...);`](jscon_scanf.md)

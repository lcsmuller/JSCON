# JSCON API Reference

### `jscon_cb;`

### Function Format

`jscon_item_t* (your_callback)(jscon_item_t*);`

### Description

A function pointer of type `jscon_callbacks_ft` is evoked everytime a [`jscon_item_t`](jscon_item_t.md) is created inside [`jscon_parse()`](jscon_parse.md) routine. The default callback can be changed to a custom `jscon_callbacks_ft` given to [`jscon_parse_cb()`](api/jscon_parse_cb.md) parameter. The [`jscon_item_t`](jscon_item_t.md) attributes **MUSTN'T** be altered by the callback, the only exception being modifying the value of a [`jscon_item_t`](jscon_item_t.md) with primitive [`type`](jscon_type.md).

### See Also

* [`jscon_parse_cb(new_cb);`](jscon_parse_cb.md)
* [`jscon_parse(buffer);`](jscon_parse.md)
* [`jscon_item_t;`](jscon_item_t.md)
* [`enum jscon_type;`](jscon_type.md)

# JSCON API Reference

### `jscon_callbacks_ft;`

### Function Format

`jscon_item_st* (your_callback)(jscon_item_st*);`

### Description

A function pointer of type `jscon_callbacks_ft` is evoked everytime a [`jscon_item_st`](jscon_item_st.md) is created inside [`jscon_parse()`](jscon_parse.md) routine. The default callback can be changed to a custom `jscon_callbacks_ft` given to [`jscon_parser_callback()`](api/jscon_parser_callback.md) parameter. The [`jscon_item_st`](jscon_item_st.md) attributes **MUSTN'T** be altered by the callback, the only exception being modifying the value of a [`jscon_item_st`](jscon_item_st.md) with primitive [`type`](jscon_type.md).

### See Also

* [`jscon_parser_callback(new_cb);`](jscon_parser_callback.md)
* [`jscon_parse(buffer);`](jscon_parse.md)
* [`jscon_item_st;`](jscon_item_st.md)
* [`enum jscon_type;`](jscon_type.md)

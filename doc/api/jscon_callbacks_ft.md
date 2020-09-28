# JSCON API Reference

### `jscon_callbacks_ft;`

### Function Format

`jscon_item_st* (your_callback)(jscon_item_st*);`

### Description

A function pointer of type `jscon_callbacks_ft;` is evoked everytime a [`jscon_item_st;`](jscon_item_st.md) is created inside [`jscon_parse(buffer);`](jscon_parse.md) routine. The default callback can be changed to your custom made one by passing your `jscon_callbacks_ft;` to [`jscon_parser_callback(new_cb);`](api/jscon_parser_callback.md) parameter. Do NOT modify any [`jscon_item_st;`](jscon_item_st.md) attributes with your callback, use it for READ-ONLY purposes, the only exception being modifying the value of a [`jscon_item_st;`](jscon_item_st.md) with primitive [`type`](jscon_type_et.md).

### See Also

* [`jscon_parser_callback(new_cb);`](jscon_parser_callback.md)
* [`jscon_parse(buffer);`](jscon_parse.md)
* [`jscon_item_st;`](jscon_item_st.md)
* [`jscon_type_et;`](jscon_type_et.md)

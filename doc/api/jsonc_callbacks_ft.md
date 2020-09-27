# JSONc API Reference

### `jsonc_callbacks_ft;`

### Function Format

`jsonc_item_st* (your_callback)(jsonc_item_st*);`

### Description

A function pointer of type `jsonc_callbacks_ft;` is evoked everytime a [`jsonc_item_st;`](jsonc_item_st.md) is created inside [`jsonc_parse(buffer);`](jsonc_parse.md) routine. The default callback can be changed to your custom made one by passing your `jsonc_callbacks_ft;` to [`jsonc_parser_callback(new_cb);`](api/jsonc_parser_callback.md) parameter. Do NOT modify any [`jsonc_item_st;`](jsonc_item_st.md) attributes with your callback, use it for READ-ONLY purposes, the only exception being modification of primitive values.

### See Also

* [`jsonc_parser_callback(new_cb);`](jsonc_parser_callback.md)
* [`jsonc_parse(buffer);`](jsonc_parse.md)
* [`jsonc_item_st;`](jsonc_item_st.md)

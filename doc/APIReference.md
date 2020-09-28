# JSCON API Reference

This document describes the public C API.

## Datatypes

### Primitive Datatypes

| Primitive | Alias | Macro |
| :--- | :--- | :--- |
|**`jscon_char_kt *`**|`char *`|`JSCON_STRING`|
|**`jscon_double_kt`**|`double`|`JSCON_NUMBER_DOUBLE`|
|**`jscon_integer_kt`**|`long long`|`JSCON_NUMBER_INTEGER`|
|**`jscon_boolean_kt`**|`bool`|`JSCON_BOOLEAN`|
| --- | --- |`JSCON_NULL`|
| --- | --- |`JSCON_UNDEFINED`|

### Composite Datatypes

| Composite | Macro |
| :--- | :--- |
|[**`jscon_composite_st`**](api/jscon_composite_st.md)|`JSCON_OBJECT or JSCON_ARRAY`|

### Enum Datatype Flags

* [`jscon_type_et;`](api/jscon_type_et.md)

### JSON wrapper struct

* [`jscon_item_st;`](api/jscon_item_st.md)

### Callbacks

* [`jscon_callbacks_ft;`](api/jscon_callbacks_ft.md)

## Functions

### Decoding Functions

* [`jscon_parse(buffer);`](api/jscon_parse.md)
* [`jscon_parser_callback(new_cb);`](api/jscon_parser_callback.md)
* [`jscon_scanf(buffer, format, ...);`](api/jscon_scanf.md)

### Encoding Functions

* [`jscon_stringify(item, type);`](api/jscon_stringify.md)

### Destructor Functions

* [`jscon_destroy(item);`](api/jscon_destroy.md)

### Manipulation Functions

#### Utility Functions

* [`jscon_next_composite_r(item, p_current_item);`](api/jscon_next_composite_r.md)
* [`jscon_next(item);`](api/jscon_next.md)
* [`jscon_clone(item);`](api/jscon_clone.md)
* [`jscon_typeof(item);`](api/jscon_typeof.md)
* [`jscon_strdup(item);`](api/jscon_strdup.md)
* [`jscon_strncpy(dest, item, n);`](api/jscon_strncpy.md)
* [`jscon_typecmp(item, type);`](api/jscon_typecmp.md)
* [`jscon_keycmp(item, key);`](api/jscon_keycmp.md)
* [`jscon_doublecmp(item, double);`](api/jscon_doublecmp.md)
* [`jscon_intcmp(item, int);`](api/jscon_intcmp.md)
* [`jscon_double_tostr(double, p_str, n_digits);`](api/jscon_double_tostr.md)

#### Getter Functions

* [`jscon_get_root(item);`](api/jscon_get_root.md)
* [`jscon_get_branch(item, key);`](api/jscon_get_branch.md)
* [`jscon_get_sibling(origin, relative_index);`](api/jscon_get_sibling.md)
* [`jscon_get_parent(item);`](api/jscon_get_parent.md)
* [`jscon_get_byindex(item, index);`](api/jscon_get_byindex.md)
* [`jscon_get_num_branch(item);`](api/jscon_get_num_branch.md)
* [`jscon_get_type(item);`](api/jscon_get_type.md)
* [`jscon_get_boolean(item);`](api/jscon_get_boolean.md)
* [`jscon_get_string(item);`](api/jscon_get_string.md)
* [`jscon_get_double(item);`](api/jscon_get_double.md)
* [`jscon_get_integer(item);`](api/jscon_get_integer.md)

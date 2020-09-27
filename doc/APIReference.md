# JSONc API Reference

This document describes the public C API.

## Datatypes

### Primitive Datatypes

| Primitive | Alias |
| :--- | :--- |
|**`jsonc_char_kt`**|`char`|
|**`jsonc_double_kt`**|`double`|
|**`jsonc_integer_kt`**|`long long`|
|**`jsonc_boolean_kt`**|`bool`|

### Composite Datatypes

* [`jsonc_composite_st;`](api/jsonc_composite_st.md)
* [`jsonc_item_st;`](api/jsonc_item_st.md)

### Enum Datatype Flags

* [`jsonc_type_et;`](api/jsonc_type_et.md)

### Callbacks

* [`jsonc_callbacks_ft;`](api/jsonc_callbacks_ft.md)

## Functions

### Decoding Functions

* [`jsonc_parse(buffer);`](api/jsonc_parse.md)
* [`jsonc_parser_callback(new_cb);`](api/jsonc_parser_callback.md)
* [`jsonc_scanf(buffer, format, ...);`](api/jsonc_scanf.md)

### Encoding Functions

* [`jsonc_stringify(item, type);`](api/jsonc_stringify.md)

### Destructor Functions

* [`jsonc_destroy(item);`](api/jsonc_destroy.md)

### Manipulation Functions

#### Utility Functions

* [`jsonc_next_composite_r(item, p_current_item);`](api/jsonc_next_composite_r.md)
* [`jsonc_next(item);`](api/jsonc_next.md)
* [`jsonc_clone(item);`](api/jsonc_clone.md)
* [`jsonc_typeof(item);`](api/jsonc_typeof.md)
* [`jsonc_strdup(item);`](api/jsonc_strdup.md)
* [`jsonc_strncpy(dest, item, n);`](api/jsonc_strncpy.md)
* [`jsonc_typecmp(item, type);`](api/jsonc_typecmp.md)
* [`jsonc_keycmp(item, key);`](api/jsonc_keycmp.md)
* [`jsonc_doublecmp(item, double);`](api/jsonc_doublecmp.md)
* [`jsonc_intcmp(item, int);`](api/jsonc_intcmp.md)
* [`jsonc_double_tostr(double, p_str, n_digits);`](api/jsonc_double_tostr.md)

#### Getter Functions

* [`jsonc_get_root(item);`](api/jsonc_get_root.md)
* [`jsonc_get_branch(item, key);`](api/jsonc_get_branch.md)
* [`jsonc_get_sibling(origin, relative_index);`](api/jsonc_get_sibling.md)
* [`jsonc_get_parent(item);`](api/jsonc_get_parent.md)
* [`jsonc_get_byindex(item, index);`](api/jsonc_get_byindex.md)
* [`jsonc_get_num_branch(item);`](api/jsonc_get_num_branch.md)
* [`jsonc_get_type(item);`](api/jsonc_get_type.md)
* [`jsonc_get_boolean(item);`](api/jsonc_get_boolean.md)
* [`jsonc_get_string(item);`](api/jsonc_get_string.md)
* [`jsonc_get_double(item);`](api/jsonc_get_double.md)
* [`jsonc_get_integer(item);`](api/jsonc_get_integer.md)

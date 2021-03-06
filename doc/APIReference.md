# JSCON API Reference

This document describes the public C API.

## Datatypes

### Structs

* [`jscon_item_t;`](api/jscon_item_t.md)

### Enums

* [`enum jscon_type;`](api/jscon_type.md)

### Callbacks

* [`jscon_cb;`](api/jscon_cb.md)

## Functions

### Decoding Functions

* [`jscon_parse(buffer);`](api/jscon_parse.md)
* [`jscon_parse_cb(new_cb);`](api/jscon_parse_cb.md)
* [`jscon_scanf(buffer, format, ...);`](api/jscon_scanf.md)

### Encoding Functions

* [`jscon_stringify(item, type);`](api/jscon_stringify.md)

### Initialization Functions

* [`jscon_null(key);`](api/jscon_null.md)
* [`jscon_boolean(key, boolean);`](api/jscon_boolean.md)
* [`jscon_integer(key, i_number);`](api/jscon_integer.md)
* [`jscon_double(key, d_number);`](api/jscon_double.md)
* [`jscon_number(key, number);`](api/jscon_number.md)
* [`jscon_string(key, string);`](api/jscon_string.md)

* [`jscon_object(key);`](api/jscon_object.md)
* [`jscon_array(key);`](api/jscon_array.md)

### Destructor Functions

* [`jscon_delete(item, key);`](api/jscon_delete.md)
* [`jscon_destroy(item);`](api/jscon_destroy.md)

### Manipulation Functions

#### Movement Functions

* [`jscon_iter_next(item);`](api/jscon_iter_next.md)
* [`jscon_iter_composite_r(item, p_current_item);`](api/jscon_iter_composite_r.md)

#### Utility Functions

* [`jscon_size(item);`](api/jscon_size.md)
* [`jscon_append(item, new_branch);`](api/jscon_append.md)
* [`jscon_dettach(item);`](api/jscon_dettach.md)
* [`jscon_clone(item);`](api/jscon_clone.md)
* [`jscon_typeof(item);`](api/jscon_typeof.md)
* [`jscon_strdup(item);`](api/jscon_strdup.md)
* [`jscon_strcpy(dest, item);`](api/jscon_strcpy.md)

#### Comparison Functions

* [`jscon_typecmp(item, type);`](api/jscon_typecmp.md)
* [`jscon_keycmp(item, key);`](api/jscon_keycmp.md)
* [`jscon_doublecmp(item, double);`](api/jscon_doublecmp.md)
* [`jscon_intcmp(item, int);`](api/jscon_intcmp.md)

#### Getter Functions

* [`jscon_get_root(item);`](api/jscon_get_root.md)
* [`jscon_get_branch(item, key);`](api/jscon_get_branch.md)
* [`jscon_get_sibling(item, relative_index);`](api/jscon_get_sibling.md)
* [`jscon_get_parent(item);`](api/jscon_get_parent.md)
* [`jscon_get_byindex(item, index);`](api/jscon_get_byindex.md)
* [`jscon_get_index(item, key);`](api/jscon_get_key_index.md)
* [`jscon_get_type(item);`](api/jscon_get_type.md)
* [`jscon_get_boolean(item);`](api/jscon_get_boolean.md)
* [`jscon_get_string(item);`](api/jscon_get_string.md)
* [`jscon_get_double(item);`](api/jscon_get_double.md)
* [`jscon_get_integer(item);`](api/jscon_get_integer.md)

#### Setter Functions

* [`jscon_set_boolean(item, boolean);`](api/jscon_set_boolean.md)
* [`jscon_set_string(item, string);`](api/jscon_set_string.md)
* [`jscon_set_double(item, double);`](api/jscon_set_double.md)
* [`jscon_set_integer(item, int);`](api/jscon_set_integer.md)

# JSCON API Reference

### `jscon_stringify(item, type);`

### Parameters

| Parameter | Type | Description |
| :--- | :--- | :--- |
|**`item`**|[`jscon_item_t *`](jscon_item_t.md)| The JSCON item to be encoded |
|**`type`**|[`enum jscon_type`](jscon_type.md)| The primitive datatype filter for encoding |

### Return Value

| Type | Description |
| :--- | :--- |
|`char *`| The resulting encoded JSON string |

### Description

The `jscon_stringify()` returns a pointer to a JSON formatted string encoded from the given [`jscon_item_t`](jscon_item_t.md). The item parameter is treated as the root, no matter its nest level. The type parameter is the filter for the primitives to be encoded. Unspecified primitive types will be ignored, set the type to `JSCON_ANY` to include every datatype. Simultaneous types can be included by placing `BITWISE OR` between them, as shown in the example.

### Example

```c
char buffer[] = "{\"a\":10, \"b\":\"yes", \"c\":false}";
jscon_item_t *root = jscon_parse(buffer); //decode json string
char *unaltered_json = jscon_stringify(root, JSCON_ANY);
char *bool_json = jscon_stringify(root, JSCON_BOOLEAN);
char *bool_str_json = jscon_stringify(root, JSCON_BOOLEAN | JSCON_STRING);

//{"a":10, "b":"yes", "c":false}
printf("%s\n", unaltered_json);

//{"c":false}
printf("%s\n", bool_json);

//{"b":"yes, "c":false}
printf("%s\n", bool_str_json);

free(unaltered_json);
free(bool_json);
free(bool_str_json);
```

### See Also

* [`jscon_item_t;`](jscon_item_t.md)
* [`enum jscon_type;`](jscon_type.md)
* [`jscon_parse(item);`](jscon_parse.md)

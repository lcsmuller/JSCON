# JSCON API Reference

### `jscon_scanf(buffer, format, ...);`

### Parameters

| Parameter | Type | Description |
| :--- | :--- | :--- |
|**`buffer`**|`char *`| The JSON string to be parsed |
|**`format`**|`char *`| The format that contains conversion specifications  |
|**`...`**|`va_list`| The list of pointers that follow format |

### Format

A format specifier for `jscon_scanf` follow this prototype:

`#key%specifier`

Where the 'key' should be replaced by the name of the key to be matched, and 'specifier' its expected type.
'null' type items can't be specified, but in such case that the expected key is assigned to one, the value will be converted.
| Specifier | Type | Qualifying Input | Null converts to |
| :--- | :--- | :--- | :--- |
|**`jd`**|`jscon_integer_kt *`| Decimal integer. |`0`|
|**`jf`**|`jscon_double_kt *`| Floating point: Decimal number containing a decimal point. |`0.0`|
|**`js`**|`jscon_char_kt *`| String of characters. |`first char set to '\0'`|
|**`jb`**|`jscon_boolean_kt *`| True or false. |`false`|
|**`ji`**|`jscon_item_st **`| A [`jscon_item_st`](jscon_item_st.md) structure. |`item with its type set to `[`JSCON_NULL`](jscon_type_et.md)``|

### Description

The `jscon_scanf(buffer, format, ...);` function reads formatted input from a JSON string, and have the matched input be parsed directly to its aligned argument. In opposite to C library's scanf, this implementation doesn't require that the given arguments are in the same order as of the keys from the string, the only requirement is that the arguments are aligned with the format specifiers. This function ignore nests, if a key assigned to a object/array, it will have all of its inner members skipped. Essentially, it only considers keys of members from the same root element level.

### Example

```c
jscon_item_st *item;
jscon_char_kt *string;
jscon_boolean_kt boolean;

char buffer[] = "{\"alpha\":[1,2,3,4], \"beta\":\"This is a string.", \"gamma\":true}";
/* order of arguments doesn't have to be the same as the json string */
jscon_scanf(buffer, "#beta%js #gamma%jb #alpha%ji", string, &boolean, &item);
```

### See Also

* [`jscon_items_st;`](jscon_item_st.md)
* [`jscon_type_et;`](jscon_type_et.md)
* [`jscon_stringify(item, type);`](jscon_stringify.md)
* [`jscon_destroy(item);`](jscon_destroy.md)

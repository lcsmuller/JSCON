# JSONc API Reference

### `jsonc_scanf(buffer, format, ...);`

### Parameters

| Parameter | Type | Description |
| :--- | :--- | :--- |
|**`buffer`**|`char *`| The JSON string to be parsed |
|**`format`**|`char *`| The format that contains conversion specifications  |
|**`...`**|`va_list`| The list of pointers that follow format |

### Format

A format specifier for `jsonc_scanf` follow this prototype:

`#key%specifier`

Where the 'key' should be replaced by the name of the key to be matched, and 'specifier' its expected type.
| Specifier | Type | Qualifying Input |
| :--- | :--- | :--- |
|**`jd`**|`jsonc_integer_kt *`| Decimal integer. |
|**`jf`**|`jsonc_double_kt *`| Floating point: Decimal number containing a decimal point. |
|**`js`**|`jsonc_char_kt *`| String of characters. |
|**`jb`**|`jsonc_boolean_kt *`| Boolean. |
|**`ji`**|`jsonc_item_st **`| A [`jsonc_item_st`](jsonc_item_st.md) structure. |

### Description

The `jsonc_scanf(buffer, format, ...);` function reads formatted input from a JSON string, and have the matched input be parsed directly to its aligned argument. In opposite to C library's scanf, this implementation doesn't require that the given arguments are in the exact order as of the keys from the string, the only requirement is that the arguments are aligned with the format specifiers. This function ignore nests, if a object or array member is found, it will have all of its contents skipped. Essentially, it only considers the input of members from the same level.

### Example

```c
jsonc_item_st *item;
jsonc_char_kt *string;
jsonc_boolean_kt boolean;

char buffer[] = "{\"alpha\":[1,2,3,4], \"beta\":\"This is a string.", \"gamma\":true}";
/* order of arguments doesn't have to be the same as the json string */
jsonc_scanf(buffer, "#beta%js #gamma%jb #alpha%ji", string, &boolean, &item);
```

### See Also

* [`jsonc_items_st;`](jsonc_item_st.md)
* [`jsonc_stringify(item, type);`](jsonc_stringify.md)
* [`jsonc_destroy(item);`](jsonc_destroy.md)

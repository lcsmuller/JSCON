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
| Specifier | Datatype | Qualifying Input | Null converts to |
| :--- | :--- | :--- | :--- |
|**`d`**|`int*`| Decimal integer. |`0`|
|**`ld`**|`long*`| Decimal integer. |`0`|
|**`lld`**|`long long*`| Decimal integer. |`0`|
|**`f`**|`float*`| Floating point: Decimal number containing a decimal point. |`0.0`|
|**`lf`**|`double*`| Floating point: Decimal number containing a decimal point. |`0.0`|
|**`c`**|`char*`| The next character. |`char '\0'`|
|**`s`**|`char*`| String of characters. |`empty string "\0"`|
|**`b`**|`bool*`| True or false. |`false`|
|**`ji`**|`jscon_item_t**`| A [`jscon_item_t`](jscon_item_t.md) structure. |`item with type set to `[`JSCON_NULL`](enum jscon_type.md)|

### Description

The `jscon_scanf(buffer, format, ...);` function reads formatted input from a JSON string, and have the matched input be parsed directly to its aligned argument. In opposite to C library's scanf, this implementation doesn't require that the given arguments are in the same order as of the keys from the string, the only requirement is that the arguments are aligned with the format specifiers. This function ignore nests, it only considers keys which are members of the same root element. In other words, only keys exactly one level below root are searched for.

### Example

```c
jscon_item_t *item;
char *string;
bool boolean;

char buffer[] = "{\"alpha\":[1,2,3,4], \"beta\":\"This is a string.", \"gamma\":true}";
/* order of arguments doesn't have to be the same as the json string */
jscon_scanf(buffer, "#beta%s #gamma%b #alpha%ji", string, &boolean, &item);
```

### See Also

* [`jscon_item_t;`](jscon_item_t.md)
* [`enum jscon_type;`](jscon_type.md)
* [`jscon_stringify(item, type);`](jscon_stringify.md)
* [`jscon_destroy(item);`](jscon_destroy.md)

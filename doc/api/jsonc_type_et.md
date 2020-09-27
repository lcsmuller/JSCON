# JSONc API Reference

### `jsonc_type_et;`

### Datatype Flags

| Flags | Code | Description |
| :--- | :--- | :--- |
|**`JSONC_UNDEFINED`**|`0`| This item is Undefined datatype |
|**`JSONC_NULL`**|`1 << 0`| This item is Null datatype |
|**`JSONC_BOOLEAN`**|`1 << 1`| This item is Boolean datatype |
|**`JSONC_NUMBER_INTEGER`**|`1 << 2`| This item is Integer datatype |
|**`JSONC_NUMBER_DOUBLE`**|`1 << 3`| This item is Double datatype |
|**`JSONC_STRING`**|`1 << 4`| This item is String datatype |
|**`JSONC_OBJECT`**|`1 << 5`| This item is Object datatype |
|**`JSONC_ARRAY`**|`1 << 6`| This item is Array datatype |

### Superset Datatype Flags

| Flags | Code | Description |
| :--- | :--- | :--- |
|**`JSONC_NUMBER`**|`JSONC_NUMBER_INTEGER` | `JSONC_NUMBER_DOUBLE`| This item datatype is a Number |
|**`JSONC_ALL`**|`USHRT_MAX`| This item datatype is defined |

### Description

The enum `jsonc_type_et` defines the item datatype, it is also used for isolating/comparing the item datatype with another type defined in a routine parameter. As seen in [`jsonc_stringify(item,type);`](jsonc_stringify.md), and [`jsonc_typecmp(item, type);`](jsonc_typecmp.md).

### See Also

* [`jsonc_stringify(item,type);`](jsonc_stringify.md)
* [`jsonc_typecmp(item,type);`](jsonc_typecmp.md)
* [`jsonc_typeof(item);`](jsonc_typeof.md)

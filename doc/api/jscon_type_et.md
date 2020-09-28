# JSCON API Reference

### `jscon_type_et;`

### Datatype Flags

| Flags | Code | Description |
| :--- | :--- | :--- |
|**`JSCON_UNDEFINED`**|`0`| This item is of Undefined datatype |
|**`JSCON_NULL`**|`1 << 0`| This item is of Null datatype |
|**`JSCON_BOOLEAN`**|`1 << 1`| This item is of Boolean datatype |
|**`JSCON_NUMBER_INTEGER`**|`1 << 2`| This item is of Integer datatype |
|**`JSCON_NUMBER_DOUBLE`**|`1 << 3`| This item is of Double datatype |
|**`JSCON_STRING`**|`1 << 4`| This item is of String datatype |
|**`JSCON_OBJECT`**|`1 << 5`| This item is of Object datatype |
|**`JSCON_ARRAY`**|`1 << 6`| This item is of Array datatype |

### Superset Datatype Flags

| Flags | Code | Description |
| :--- | :--- | :--- |
|**`JSCON_NUMBER`**|`JSCON_NUMBER_INTEGER + JSCON_NUMBER_DOUBLE`| This item datatype is a Number |
|**`JSCON_ANY`**|`USHRT_MAX`| This item datatype is not Undefined |

### Description

The enum `jscon_type_et` defines the item datatype, it is also used for filtering or comparing the item datatype with another type defined in a routine parameter. As seen in [`jscon_stringify(item,type);`](jscon_stringify.md) and [`jscon_typecmp(item, type);`](jscon_typecmp.md), respectively.

### See Also

* [`jscon_stringify(item,type);`](jscon_stringify.md)
* [`jscon_typecmp(item,type);`](jscon_typecmp.md)
* [`jscon_typeof(item);`](jscon_typeof.md)

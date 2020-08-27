#include "../JSON.h"

#include <stdlib.h>
#include <stdio.h>

/* converts json item into a string and returns its address */
json_string_kt json_item_stringify(json_item_st *root, json_type_et type);

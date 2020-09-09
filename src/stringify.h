#ifndef JSONC_STRINGIFY_H
#define JSONC_STRINGIFY_H

struct jsonc_item_s; //forward declaration

char* jsonc_stringify(struct jsonc_item_s *root, ushort type);

#endif

#ifndef JSONC_PARSER_H
#define JSONC_PARSER_H

struct jsonc_item_s; //forward declaration

/* parse buffer and returns a jsonc item */
struct jsonc_item_s* jsonc_parse(char *buffer);
/* same, but with a user created function that can manipulate
  the parsing contents */
//@todo: replace this with a write callback modifier, make a default write callback
struct jsonc_item_s* jsonc_parse_reviver(char *buffer, void (*fn)(struct jsonc_item_s*));
/* clean up jsonc item and global allocated keys */
void jsonc_destroy(struct jsonc_item_s *item);

#endif

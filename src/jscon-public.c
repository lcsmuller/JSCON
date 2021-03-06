/*
 * Copyright (c) 2020 Lucas Müller
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libjscon.h>

#include "jscon-common.h"
#include "debug.h"
#include "strscpy.h"


static inline jscon_item_t*
_jscon_new(const char *key, enum jscon_type type)
{
    jscon_item_t *new_item = malloc(sizeof *new_item);
    if (NULL == new_item) return NULL;

    if (NULL != key){
        new_item->key = strdup(key);
        if (NULL == new_item->key){
            free(new_item);
            return NULL;
        }
    } else {
        new_item->key = NULL;
    }

    new_item->parent = NULL;
    new_item->type = type;

    return new_item;
}

jscon_item_t*
jscon_null(const char *key){
    return _jscon_new(key, JSCON_NULL);
}

jscon_item_t*
jscon_boolean(const char *key, bool boolean)
{
    jscon_item_t *new_item = _jscon_new(key, JSCON_BOOLEAN);
    if (NULL == new_item) return NULL;

    new_item->boolean = boolean;

    return new_item;
}

jscon_item_t*
jscon_integer(const char *key, long long i_number)
{
    jscon_item_t *new_item = _jscon_new(key, JSCON_INTEGER);
    if (NULL == new_item) return NULL;

    new_item->i_number = i_number;

    return new_item;
}

jscon_item_t*
jscon_double(const char *key, double d_number)
{
    jscon_item_t *new_item = _jscon_new(key, JSCON_DOUBLE);
    if (NULL == new_item) return NULL;

    new_item->d_number = d_number;

    return new_item;
}

jscon_item_t*
jscon_number(const char *key, double number)
{
    return DOUBLE_IS_INTEGER(number)
            ? jscon_integer(key, (long long)number)
            : jscon_double(key, number);
}

jscon_item_t*
jscon_string(const char *key, char *string)
{
    if (NULL == string) return jscon_null(key);

    jscon_item_t *new_item = _jscon_new(key, JSCON_STRING);
    if (NULL == new_item) return NULL;

    new_item->string = strdup(string);
    if (NULL == new_item->string) goto cleanupA;

    return new_item;

cleanupA:
    free(new_item->key);
    free(new_item);

    return NULL;
}

inline static jscon_item_t*
_jscon_composite(const char *key, enum jscon_type type)
{
    jscon_item_t *new_item = _jscon_new(key, type);
    if (NULL == new_item) return NULL;

    new_item->comp = calloc(1, sizeof *new_item->comp);
    if (NULL == new_item->comp) goto cleanupA;

    new_item->comp->hashtable = hashtable_init(); 
    if (NULL == new_item->comp->hashtable) goto cleanupB;

    new_item->comp->branch = malloc(sizeof(jscon_item_t*));
    if (NULL == new_item->comp->branch) goto cleanupC;

    Jscon_composite_build(new_item);

    return new_item;


cleanupC:
    hashtable_destroy(new_item->comp->hashtable);
cleanupB:
    free(new_item->comp);
cleanupA:
    free(new_item->key);
    free(new_item);

    return NULL;
}

jscon_item_t*
jscon_object(const char *key){
    return _jscon_composite(key, JSCON_OBJECT);
}

jscon_item_t*
jscon_array(const char *key){
    return _jscon_composite(key, JSCON_ARRAY);
}

/* total branches the item possess, returns 0 if item type is primitive */
size_t
jscon_size(const jscon_item_t *item){
    return IS_COMPOSITE(item) ? item->comp->num_branch : 0;
} 

static size_t
_jscon_depth(jscon_item_t *item)
{
    size_t depth = 0;
    while (!IS_ROOT(item)){
        item = item->parent;
        ++depth;
    }

    return depth;
}

/* get the last comp relative to the item */
static jscon_composite_t*
_jscon_get_deepest(jscon_item_t *item)
{
    ASSERT_S(IS_COMPOSITE(item), jscon_strerror(JSCON_EXT__NOT_COMPOSITE, item));

    size_t item_depth = _jscon_depth(item);

    /* get the deepest nested composite relative to item */
    jscon_composite_t *comp_last = item->comp;
    while(NULL != comp_last->next){
        if (item_depth > _jscon_depth(comp_last->next->p_item)){
            break;
        }
        comp_last = comp_last->next;
    }

    return comp_last;
}

jscon_item_t*
jscon_append(jscon_item_t *item, jscon_item_t *new_branch)
{
    ASSERT_S(new_branch != item, "Can't perform circular append");

    char *hold_key = NULL; /* hold new_branch->key incase we can't allocate memory for new numerical key */
    switch (item->type){
    case JSCON_ARRAY:
     {
        hold_key = new_branch->key; 

        char numkey[MAX_INTEGER_DIG];
        snprintf(numkey, MAX_INTEGER_DIG-1, "%zu", item->comp->num_branch);

        new_branch->key = strdup(numkey);
        if (NULL == new_branch->key) goto cleanupA; /* Out of memory, reattach its old key and return NULL */
     }
    /* fall through */
    case JSCON_OBJECT:
        break;
    default:
        ERROR("Can't append to\n\t%s", jscon_strerror(JSCON_EXT__NOT_COMPOSITE, item));
    }

    /* realloc parent references to match new size */
    jscon_item_t **tmp = realloc(item->comp->branch, (1+item->comp->num_branch) * sizeof(jscon_item_t*));
    if (NULL == tmp) goto cleanupB;

    item->comp->branch = tmp;

    ++item->comp->num_branch;

    item->comp->branch[item->comp->num_branch-1] = new_branch;
    new_branch->parent = item;

    if (item->comp->num_branch <= item->comp->hashtable->num_bucket){
        Jscon_composite_set(new_branch->key, new_branch);
    } else {
        Jscon_composite_remake(item);
    }

    if (IS_COMPOSITE(new_branch)){
        /* get the last comp relative to item */
        jscon_composite_t *comp_last = _jscon_get_deepest(item);
        comp_last->next = new_branch->comp;
        new_branch->comp->prev = comp_last;
    }

    if (hold_key != NULL){
        free(hold_key);
    }

    return new_branch;

cleanupB:
    if (hold_key != NULL)
        free(new_branch->key);
cleanupA:
    new_branch->key = hold_key;

    return NULL;
}

/* @todo test this */
jscon_item_t*
jscon_dettach(jscon_item_t *item)
{
    /* can't dettach root from nothing */
    if (NULL == item || IS_ROOT(item)) return item;

    /* get the item index reference from its parent */
    jscon_item_t *item_parent = item->parent;

    /* realloc parent references to match new size */
    jscon_item_t **tmp = realloc(item_parent->comp->branch, jscon_size(item_parent) * sizeof(jscon_item_t*));
    if (NULL == tmp) return NULL;

    item_parent->comp->branch = tmp;

    /* dettach the item from its parent and reorder keys */
    for (size_t i = jscon_get_index(item_parent, item->key); i < jscon_size(item_parent)-1; ++i){
        item_parent->comp->branch[i] = item_parent->comp->branch[i+1]; 
    }
    item_parent->comp->branch[jscon_size(item_parent)-1] = NULL;
    --item_parent->comp->num_branch;

    /* parent hashtable has to be remade, to match reordered keys */
    Jscon_composite_remake(item_parent);

    /* get the immediate previous comp relative to the item */
    jscon_composite_t *comp_prev = item->comp->prev;
    /* get the last comp relative to item */
    jscon_composite_t *comp_last = _jscon_get_deepest(item);

    /* remove tree references to the item */
    comp_prev->next = comp_last->next;

    /* remove item references to the tree */
    item->parent = NULL;
    comp_last->next = NULL;
    item->comp->prev = NULL;

    return item;
}

void
jscon_delete(jscon_item_t *item, const char *key)
{
    jscon_item_t *branch = jscon_get_branch(item, key);
    if (NULL == branch) return;

    jscon_dettach(branch); 
    jscon_destroy(branch);
}


/* reentrant function, works similar to strtok. the starting point is set
 *  by doing the function call before the main iteration loop, then
 *  consecutive function calls inside the loop will continue the iteration
 *  from then on if item is then set to NULL.  
 *  p_current_item allows for thread safety reentrancy, it should not be
 *  modified outside of this routine*/
jscon_item_t*
jscon_iter_composite_r(jscon_item_t *item, jscon_item_t **p_current_item)
{
    if (item != NULL){
        /* set p_current_item to item, otherwise fetch next iteration */ 
        ASSERT_S(IS_COMPOSITE(item), jscon_strerror(JSCON_EXT__NOT_COMPOSITE, item));
        return *p_current_item = item;
    }

    if (NULL == *p_current_item){
        /* p_current_item needs to be set with non-NULL item parameter */
        return NULL;
    }

    /* get next comp in line, if NULL it means there are no more
     *  composite datatype items to iterate through */
    jscon_composite_t *next_comp = (*p_current_item)->comp->next;
    if (NULL == next_comp){
        /* reach end of composite items */
        return *p_current_item = NULL;
    }

    /* update current_item and return next composite */
    return *p_current_item = next_comp->p_item;
}

/* return next (not yet accessed) item, by using item->comp->last_accessed_branch as the branch index */
static inline jscon_item_t*
_jscon_push(jscon_item_t *item)
{
    ASSERT_S(IS_COMPOSITE(item), jscon_strerror(JSCON_EXT__NOT_COMPOSITE, item));
    ASSERT_S(item->comp->last_accessed_branch < item->comp->num_branch, jscon_strerror(JSCON_INT__OVERFLOW, item->comp));

    ++item->comp->last_accessed_branch; /* update last_accessed_branch to next */
    jscon_item_t *next_item = item->comp->branch[item->comp->last_accessed_branch-1];

    if (IS_COMPOSITE(next_item)){
        /* resets next_item that might have been set from a different run */
        next_item->comp->last_accessed_branch = 0;
    }

    return next_item;
}

static inline jscon_item_t*
_jscon_pop(jscon_item_t *item)
{
    if (IS_COMPOSITE(item)){
        /* resets object's last_accessed_branch */
        item->comp->last_accessed_branch = 0;
    }

    return item->parent;
}

/* this will simulate tree preorder traversal iteratively, by using 
 *  item->comp->last_accessed_branch like a stack frame. under no circumstance 
 *  should you modify last_accessed_branch value directly */
jscon_item_t*
jscon_iter_next(jscon_item_t *item)
{
    if (NULL == item) return NULL;

    /* resets root's last_accessed_branch in case its set from a different run */
    if (IS_COMPOSITE(item)){
        item->comp->last_accessed_branch = 0;
    }

    if (IS_LEAF(item)){
        /* item is a leaf, fetch parent until found a item with any branch
         *  left to be accessed */
        do {
            /* fetch parent until a item with unacessed branch is found */
            item = _jscon_pop(item);
            if ((NULL == item) || (0 == item->comp->last_accessed_branch)){
                /* item is unexistent (root's parent) or all of 
                 *  its branches have been accessed */
                return NULL;
            }
        } while (item->comp->num_branch == item->comp->last_accessed_branch);
    }

    return _jscon_push(item);
}

/* This is not the most effective way to clone a item, but it is
 *  the most reliable, because it automatically accounts for any
 *  new feature that might added in the future. By first stringfying the
 *  (to be cloned) Item and then parsing the resulting string into
 *  a new clone Item, it's guaranteed that it will be a perfect 
 *  clone, with its unique hashtable, strings, etc */
jscon_item_t*
jscon_clone(jscon_item_t *item)
{
    if (NULL == item) return NULL;

    char *tmp_buffer = jscon_stringify(item, JSCON_ANY);
    jscon_item_t *clone = jscon_parse(tmp_buffer);
    free(tmp_buffer);

    if (NULL != item->key){
        clone->key = strdup(item->key);
        if (NULL == clone->key){
            jscon_destroy(clone);
            clone = NULL;
        }
    }

    return clone;
}

char*
jscon_typeof(const jscon_item_t *item)
{
/* if case matches, return token as string */
#define CASE_RETURN_STR(type) case type: return #type

    if (NULL == item) return "JSCON_UNDEFINED";

    switch (item->type){
    CASE_RETURN_STR(JSCON_DOUBLE);
    CASE_RETURN_STR(JSCON_INTEGER);
    CASE_RETURN_STR(JSCON_STRING);
    CASE_RETURN_STR(JSCON_NULL);
    CASE_RETURN_STR(JSCON_BOOLEAN);
    CASE_RETURN_STR(JSCON_OBJECT);
    CASE_RETURN_STR(JSCON_ARRAY);
    CASE_RETURN_STR(JSCON_UNDEFINED);

    default: return "JSCON_UNDEFINED";
    }
}

char*
jscon_strdup(const jscon_item_t *item)
{
    char *src = jscon_get_string(item);
    if (NULL == src) return NULL;

    char *dest = strdup(src);

    return dest;
}

char*
jscon_strcpy(char *dest, const jscon_item_t *item)
{
    char *src = jscon_get_string(item);
    if (NULL == src) return NULL;

    strscpy(dest, src, strlen(src)+1);

    return dest;
}

int
jscon_typecmp(const jscon_item_t *item, const enum jscon_type type){
    return item->type & type; /* BITMASK AND */
}

int
jscon_keycmp(const jscon_item_t *item, const char *key){
    return (NULL != item->key) ? STREQ(item->key, key) : 0;
}

int
jscon_doublecmp(const jscon_item_t *item, const double d_number){
    ASSERT_S(JSCON_DOUBLE == item->type, jscon_strerror(JSCON_EXT__NOT_NUMBER, (void*)item));

    return item->d_number == d_number;
}

int
jscon_intcmp(const jscon_item_t *item, const long long i_number){
    ASSERT_S(JSCON_INTEGER == item->type, jscon_strerror(JSCON_EXT__NOT_NUMBER, (void*)item));

    return item->i_number == i_number;
}

jscon_item_t*
jscon_get_root(jscon_item_t *item)
{
    while (!IS_ROOT(item)){
        item = item->parent;
    }

    return item;
}


/* get item branch with given key */
jscon_item_t*
jscon_get_branch(jscon_item_t *item, const char *key)
{
    ASSERT_S(IS_COMPOSITE(item), jscon_strerror(JSCON_EXT__NOT_COMPOSITE, item));

    if (NULL == key) return NULL;

    /* search for entry with given key at item's comp,
      and retrieve found or not found(NULL) item */
    return Jscon_composite_get(key, item);
}

/* get origin item sibling by the relative index, if origin item is of index 3 (from parent's perspective), and relative index is -1, then this function will return item of index 2 (from parent's perspective) */
jscon_item_t*
jscon_get_sibling(const jscon_item_t* item, const size_t relative_index)
{
    ASSERT_S(!IS_ROOT(item), "Item is root (has no siblings)");

    /* get parent's branch index of the origin item */
    size_t item_index = jscon_get_index(item->parent, item->key);

    if ((0 <= (int)(item_index + relative_index)) 
        && jscon_size(item->parent) > (item_index + relative_index)){
        /* relative index given doesn't exceed parent's total branches, and is greater than 0 */
        return jscon_get_byindex(item->parent, item_index + relative_index);
    }

    return NULL;
}

jscon_item_t*
jscon_get_parent(const jscon_item_t *item){
    return item->parent;
}

jscon_item_t*
jscon_get_byindex(const jscon_item_t *item, const size_t index)
{
    ASSERT_S(IS_COMPOSITE(item), jscon_strerror(JSCON_EXT__NOT_COMPOSITE, (void*)item));
    return (index < item->comp->num_branch) ? item->comp->branch[index] : NULL;
}

long
jscon_get_index(const jscon_item_t *item, const char *key)
{
    ASSERT_S(IS_COMPOSITE(item), jscon_strerror(JSCON_EXT__NOT_COMPOSITE, (void*)item));

    jscon_item_t *lookup_item = Jscon_composite_get(key, (jscon_item_t*)item);

    if (NULL == lookup_item) return -1;

    /* @todo currently this is O(n), a possible alternative
     *  is adding a new attribute that stores the item's index */
    for (size_t i=0; i < item->comp->num_branch; ++i){
        if (lookup_item == item->comp->branch[i]){
            return i;
        }
    }

    ERROR("Item exists in hashtable but is not referenced by parent");
    abort();
}

enum jscon_type
jscon_get_type(const jscon_item_t *item){
    return (NULL != item) ? item->type : JSCON_UNDEFINED;
}

char*
jscon_get_key(const jscon_item_t *item){
    return (NULL != item) ? item->key : NULL;
}

bool
jscon_get_boolean(const jscon_item_t *item)
{
    if (NULL == item || JSCON_NULL == item->type) return false;

    ASSERT_S(JSCON_BOOLEAN == item->type, jscon_strerror(JSCON_EXT__NOT_BOOLEAN, (void*)item));
    return item->boolean;
}

char*
jscon_get_string(const jscon_item_t *item)
{
    if (NULL == item || JSCON_NULL == item->type) return NULL;

    ASSERT_S(JSCON_STRING == item->type, jscon_strerror(JSCON_EXT__NOT_STRING, (void*)item));
    return item->string;
}

double
jscon_get_double(const jscon_item_t *item)
{
    if (NULL == item || JSCON_NULL == item->type) return 0.0;

    ASSERT_S(JSCON_DOUBLE == item->type, jscon_strerror(JSCON_EXT__NOT_NUMBER, (void*)item));
    return item->d_number;
}

long long
jscon_get_integer(const jscon_item_t *item)
{
    if (NULL == item || JSCON_NULL == item->type) return 0;

    ASSERT_S(JSCON_INTEGER == item->type, jscon_strerror(JSCON_EXT__NOT_NUMBER, (void*)item));
    return item->i_number;
}

jscon_item_t*
jscon_set_boolean(jscon_item_t *item, bool boolean)
{
    item->boolean = boolean;
    return item;
}

jscon_item_t*
jscon_set_string(jscon_item_t *item, char *string)
{
    if (item->string){
      free(item->string);
    }

    item->string = strdup(string);
    return item;
}

jscon_item_t*
jscon_set_double(jscon_item_t *item, double d_number)
{
    item->d_number = d_number;
    return item;
}

jscon_item_t*
jscon_set_integer(jscon_item_t *item, long long i_number)
{
    item->i_number = i_number;
    return item;
}

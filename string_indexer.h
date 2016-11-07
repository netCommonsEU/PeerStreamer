#ifndef __STRING_INDEXER_H__
#define __STRING_INDEXER_H__ 1

#include <stdint.h>

struct string_indexer * string_indexer_new(const uint32_t size);

void string_indexer_destroy(struct string_indexer ** si);

uint32_t string_indexer_id(struct string_indexer * si,const char* s);

uint8_t string_indexer_check(const struct string_indexer * si,const char* s);

uint32_t string_indexer_size(struct string_indexer *si);

#endif

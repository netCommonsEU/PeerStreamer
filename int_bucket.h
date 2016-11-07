#ifndef __INT_BUCKET_H__
#define __INT_BUCKET_H__ 1

#include <stdint.h>
#include "sparse_vector.h"

#define INT_BUCKET_INC_SIZE 10

struct int_bucket * int_bucket_new(const uint32_t size);

struct sparse_vector * int_bucket_2_sparse_vector(const struct int_bucket *ib);

void int_bucket_destroy(struct int_bucket **ib);

int int_bucket_insert(struct int_bucket * ib,const uint32_t n,const uint32_t occurr);

void int_bucket_sum(struct int_bucket *dst, const struct int_bucket *op);

double int_bucket_occurr_norm(const struct int_bucket *dst);

uint32_t int_bucket_length(const struct int_bucket *ib);

#endif

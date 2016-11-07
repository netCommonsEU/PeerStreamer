#ifndef __SPARSE_VECTOR_H__
#define __SPARSE_VECTOR_H__ 1

#include <stdint.h>

#define SPARSE_VECTOR_INC_SIZE 10

struct sparse_vector * sparse_vector_new(const uint32_t size);

void sparse_vector_destroy(struct sparse_vector **v);

int sparse_vector_set_element(struct sparse_vector * v,const uint32_t n,const double value);

void sparse_vector_sum(struct sparse_vector *dst, const struct sparse_vector *op);

double sparse_vector_norm(const struct sparse_vector *dst);

void sparse_vector_multiply(struct sparse_vector *v, const double scalar);

#endif

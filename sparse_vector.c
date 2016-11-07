#include <math.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>

#include "sparse_vector.h"

struct sparse_vector {
	double *value;
	uint32_t *position;
	uint32_t size;
	uint32_t n_elements;
};

uint32_t sparse_vector_check_pos(const struct sparse_vector *h, const uint32_t n)
{
	uint32_t a,b,i;

	i = 0;
	if(h && h->n_elements > 0)
	{
		a = 0;
		b = h->n_elements-1;
		i = (b+a)/2;
		
		while(b > a)
		{
			if (n > h->position[i])
				a = i+1;
			if (n < h->position[i])
				b = i ? i-1 : 0;
			if (n == h->position[i])
				a = b = i;
		
			i = (b+a)/2;
		}
		if (n > (h->position[i]))
			i++;
	}

	return i;	
}

int sparse_vector_init(struct sparse_vector *v,const uint32_t size)
{
	if(v)
	{
		v->size = size;
		v->value = (double *) malloc(v->size * sizeof(double));
		v->position = (uint32_t *) malloc(v->size * sizeof(uint32_t));
		v->n_elements = 0;
	}
	return v->value == NULL && v->position == NULL ? -1 : 0;
}

struct sparse_vector * sparse_vector_new(const uint32_t size)
{
	struct sparse_vector * v = NULL;
	v = (struct sparse_vector *) malloc(sizeof(struct sparse_vector));
	if (v)
		sparse_vector_init(v,size);
	return v;
}

void sparse_vector_destroy(struct sparse_vector **v)
{
	if(*v)
	{
		if(*v != NULL && (*v)->value != NULL)
			free(((*v)->value));
		if(*v != NULL && (*v)->position != NULL)
			free(((*v)->position));
		free(*v);
		*v=NULL;
	}
}

int sparse_vector_set_element(struct sparse_vector * v,const uint32_t n,const double scalar)
{
	uint32_t i;

	if(v)
	{
		i = sparse_vector_check_pos(v,n);
		if(i>= v->n_elements || v->position[i] != n)
		{
			if((v->n_elements + 1) >= v->size)
			{
				v->size += SPARSE_VECTOR_INC_SIZE;
				v->value = (double *) realloc(v->value,sizeof(double) * v->size);
				v->position = (uint32_t *) realloc(v->position,sizeof(uint32_t) * v->size);
			}
			memmove(v->position + i+1,v->position + i,sizeof(uint32_t) * (v->n_elements -i));
			memmove(v->value + i+1,v->value + i,sizeof(double) * (v->n_elements -i));

			v->position[i] = n;
			v->value[i] = scalar;
			v->n_elements++;
		} else
			v->value[i] = scalar;
		return 0;
	}
	return -1;
}

void sparse_vector_sum(struct sparse_vector *dst, const struct sparse_vector *op)
{
	uint32_t i,j;
	if (dst && op)
		for (i = 0; i<op->n_elements;i++)
		{
			j = sparse_vector_check_pos(dst,op->position[i]);
			if(j>= dst->n_elements || dst->position[j] != op->position[i])
				sparse_vector_set_element(dst,op->position[i],op->value[i]);
			else
				dst->value[j] += op->value[i];
		}
}

double sparse_vector_norm(const struct sparse_vector *v)
{
	double sum = 0;
	uint32_t i;

	sum = 0;
	for (i = 0; v && i<v->n_elements; i++)
	{
//		fprintf(stderr,"Element %d with value %f\n",v->position[i],v->value[i]);
		sum += (v->value[i])*(v->value[i]);
	}

	return sqrt(sum);
}

void sparse_vector_multiply(struct sparse_vector *v, const double scalar)
{
	uint32_t i;

	for (i = 0; v && i<v->n_elements; i++)
		v->value[i] *= scalar;
}

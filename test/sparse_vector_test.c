#include<malloc.h>
#include<assert.h>

#include"sparse_vector.h"

void sparse_vector_init_test()
{

	struct sparse_vector * v;

	v = sparse_vector_new(0);
	sparse_vector_destroy(&v);
	assert(v==NULL);

	v = sparse_vector_new(3);
	sparse_vector_destroy(&v);
	assert(v==NULL);

	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void sparse_vector_set_element_test()
{
	struct sparse_vector * v;

	assert(sparse_vector_set_element(NULL,1,1) < 0);

	v = sparse_vector_new(0);

	assert(0 == sparse_vector_set_element(v,1,1));
	assert(sparse_vector_norm(v) == 1);

	assert(0 == sparse_vector_set_element(v,1,1));
	assert(sparse_vector_norm(v) == 1);

	assert(0 == sparse_vector_set_element(v,2,1));
	assert(sparse_vector_norm(v) < 1.43);
	assert(sparse_vector_norm(v) > 1.4);

	assert(0 == sparse_vector_set_element(v,3,4));
	assert(sparse_vector_norm(v) < 4.3);
	assert(sparse_vector_norm(v) > 4.2);

	assert(0 == sparse_vector_set_element(v,2,1));
	assert(sparse_vector_norm(v) < 4.3);
	assert(sparse_vector_norm(v) > 4.2);

	sparse_vector_destroy(&v);

	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void sparse_vector_norm_test()
{
	struct sparse_vector * v;

	assert(sparse_vector_norm(NULL) == 0);

	v = sparse_vector_new(0);
	assert(0 == sparse_vector_set_element(v,1,1));
	assert(sparse_vector_norm(v) == 1);

	assert(0 == sparse_vector_set_element(v,1,3));
	assert(sparse_vector_norm(v) == 3);

	assert(0 == sparse_vector_set_element(v,10,4));
	assert(sparse_vector_norm(v) == 5);

	assert(0 == sparse_vector_set_element(v,343,7));
	assert(sparse_vector_norm(v) < 8.61);
	assert(sparse_vector_norm(v) > 8.6);

	sparse_vector_destroy(&v);

	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void sparse_vector_sum_test()
{
	struct sparse_vector * v1,* v2;

	sparse_vector_sum(NULL,NULL);

	v1 = sparse_vector_new(0);
	sparse_vector_sum(v1,NULL);
	sparse_vector_sum(NULL,v1);

	v2 = sparse_vector_new(0);
	sparse_vector_sum(v1,v2);
	assert(sparse_vector_norm(v1) == 0);

	sparse_vector_set_element(v2,5,3);
	sparse_vector_set_element(v2,67,2);
	sparse_vector_set_element(v1,67,2);
	sparse_vector_sum(v1,v2);
	assert(sparse_vector_norm(v1) == 5);

	sparse_vector_destroy(&v1);
	sparse_vector_destroy(&v2);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void sparse_vector_multiply_test()
{
	struct sparse_vector * v1;

	v1 = sparse_vector_new(10);
	sparse_vector_set_element(v1,1,3);
	sparse_vector_set_element(v1,10,4);

	assert(sparse_vector_norm(v1) == 5);

	sparse_vector_multiply(v1, 1);
	assert(sparse_vector_norm(v1) == 5);

	sparse_vector_multiply(v1, 2);
	assert(sparse_vector_norm(v1) == 10);

	sparse_vector_multiply(v1, 0);
	assert(sparse_vector_norm(v1) == 0);

	sparse_vector_destroy(&v1);

	fprintf(stderr,"%s successfully passed!\n",__func__);
}

int main(char ** argc,int argv)
{
	sparse_vector_init_test();
	sparse_vector_set_element_test();
	sparse_vector_norm_test();
	sparse_vector_sum_test();
	sparse_vector_multiply_test();
	return 0;
}

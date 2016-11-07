#include <assert.h>
#include <stdio.h>
#include <string_indexer.h>

void string_indexer_new_test()
{
	struct string_indexer * si;

	si = string_indexer_new(0);
	assert(string_indexer_size(si) == 0);
	string_indexer_destroy(&si);

	si = string_indexer_new(30);
	assert(string_indexer_size(si) == 0);
	string_indexer_destroy(&si);

	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void string_indexer_id_test()
{
	struct string_indexer *si = NULL;
	uint32_t i;

	string_indexer_id(NULL,"ciao");

	si = string_indexer_new(0);
	string_indexer_id(si,NULL);
	assert(string_indexer_size(si) == 0);

	i = string_indexer_id(si,"ciao");
	assert(string_indexer_size(si) == 1);
	assert(string_indexer_id(si,"ciao") == i);
	assert(string_indexer_size(si) == 1);

	assert(i != string_indexer_id(si,"hole"));
	assert(string_indexer_id(si,"ciao") == i);
	assert(string_indexer_size(si) == 2);

	string_indexer_destroy(&si);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void string_indexer_size_test()
{
	struct string_indexer *si = NULL;

	assert(0 == string_indexer_size(NULL));

	string_indexer_destroy(&si);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}
int main(char ** argc,int argv)
{
	string_indexer_new_test();
	string_indexer_id_test();
	string_indexer_size_test();
	return 0;
}

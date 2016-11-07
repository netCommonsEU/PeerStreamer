#include <stdint.h>
#include <string.h>
#include <malloc.h>

#define STRING_INDEXER_INC_SIZE 10

struct string_indexer {
	char ** strings;
	uint32_t * ids;
	uint32_t n_elements;
	uint32_t size;
};

int string_indexer_init(struct string_indexer * si,const uint32_t size)
{
	if(si)
	{
		si->n_elements = 0;
		si->size = size;
		si->strings = (char **) malloc (sizeof(char *) * si->size);
		si->ids = (uint32_t *) malloc (sizeof(char *) * si->size);
	}
	return si->strings == NULL || si->ids == NULL ? -1 : 0;
}

struct string_indexer * string_indexer_new(const uint32_t size)
{
	struct string_indexer * si = NULL;

	si = (struct string_indexer *) malloc (sizeof(struct string_indexer));

	string_indexer_init(si,size);
	return si;
}

void string_indexer_destroy(struct string_indexer ** si)
{
	uint32_t i;
	if(*si && (*si)->strings)
	{
		for(i = 0; i < (*si)->n_elements; i++)
			free(((*si)->strings)[i]);
		free((*si)->strings);
	}
	if(*si && (*si)->ids)
		free((*si)->ids);
	if(*si)
		free(*si);
	*si = NULL;
}

uint32_t string_indexer_check_pos(const struct string_indexer *h, const char * s)
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
			if (strcmp(s,h->strings[i]) > 0)
				a = i+1;
			if (strcmp(s,h->strings[i]) < 0)
				b = i ? i-1 : 0;
			if (strcmp(s,h->strings[i]) == 0)
				a = b = i;
		
			i = (b+a)/2;
		}
		if (strcmp(s,h->strings[i]) > 0)
			i++;
	}

	return i;	
}

uint32_t string_indexer_id(struct string_indexer * si,const char * line)
{
	uint32_t i;

	if(si && line)
	{
		i = string_indexer_check_pos(si,line);
		if(i>= si->n_elements || strcmp(line,(si->strings)[i]))
		{
			if((si->n_elements + 1) >= si->size)
			{
				si->size += STRING_INDEXER_INC_SIZE;
				si->strings = (char **) realloc(si->strings,sizeof(char*) * si->size);
				si->ids = (uint32_t *) realloc(si->ids,sizeof(uint32_t) * si->size);
			}
			memmove(&(si->strings[i+1]),&(si->strings[i]),sizeof(char *) * (si->n_elements -i));
			memmove(&(si->ids[i+1]),&(si->ids[i]),sizeof(uint32_t) * (si->n_elements -i));

			si->strings[i] = strdup(line);
			si->ids[i] = si->n_elements;
			si->n_elements++;
		} 
		return si->ids[i];
	}
	return 0;
}

uint32_t string_indexer_size(struct string_indexer *si)
{
	if(si)
		return si->n_elements;
	else
		return 0;
}

uint8_t string_indexer_check(const struct string_indexer * si,const char* s)
{
	uint32_t pos;

	pos = string_indexer_check_pos(si,s);

	if(pos >= si->n_elements || strcmp(si->strings[pos],s) != 0)
		return 0;
	return 1;
}

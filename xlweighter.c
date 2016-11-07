#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

#include<grapes_config.h>

#include "string_indexer.h"
#include "int_bucket.h"
#include "sparse_vector.h"
#include "xlweighter.h"

#define INT_BUCKET_INC_SIZE 10

struct XLayerWeighter {
	struct int_bucket ** paths;
	uint32_t * paths_id;
	uint32_t n_paths;
	uint32_t size;
	uint32_t nodes_num;
	struct string_indexer * nodes_names;
	struct sparse_vector * paths_sum;
	int hopcount_only;
	int expected_degree;
};

char * substring_trim(const char * s,uint32_t start,uint32_t len)
{
	uint32_t leading=0,trailing=0;

	while(leading < len && (s[start+leading] == '\n' || s[start+leading] == ' '))
		leading++;
	while(trailing < len && (s[start+len-trailing-1] == '\n' || s[start+len-trailing-1] == ' '))
		trailing++;

	if(len - leading -trailing > 0)
		return strndup(s+start+leading,len-trailing);
	else
		return NULL;
}

uint32_t triel(uint32_t n,uint32_t a,uint32_t b)
{
	int64_t i,j;
	uint32_t k;

	i = a>b ? b : a;
	j = a>b ? a : b;
	k = j-1 + ((2*n-3)*i-i*i)/2; 
	return k;
}

void tokens_destroy(char *** sv,uint32_t n)
{
	uint32_t i;

	for( i = 0; i < n; i++)
		free((*sv)[i]);
	free(*sv);
	*sv = NULL;
}

char ** tokens_create(char * s,const char delim,uint32_t * n)
{
	char * np =s, *wp = s;
	char ** sv = NULL;
	uint32_t i;

	*n = 0;

	if(s && strcmp(s,""))
	{
		while((np = strchr(np,delim)))
		{
			(*n)++;
			np++;
		}
		(*n)++;
		sv = (char **) malloc(sizeof(char *) * (*n));
	}

	np = s;
	for( i=0; i<*n; i++)
	{
		np = strchr(np,delim);
		if(np)
		{
			sv[i] = substring_trim(wp,0,np-wp);
			np++;
			wp=np;
		} else
			sv[i] = substring_trim(wp,0,strlen(wp));
	}
	return sv;
}

uint32_t xlweighter_parse_names(const struct XLayerWeighter * xlw,FILE *fp)
{
	char line[MAX_PATH_STRING_LENGTH];
	char ** tokens;
	uint32_t tok_num,i;

	if(fp)
	{
		while( fgets(line,MAX_PATH_STRING_LENGTH,fp) != NULL)
		{
			tokens = tokens_create(line,',',&tok_num);
			for (i = 0; i<tok_num; i++)
			{
				//fprintf(stderr,"[INFO] adding name *%s*\n",tokens[i]);
				string_indexer_id(xlw->nodes_names,tokens[i]);
			}
			tokens_destroy(&tokens,tok_num);
		}

		return string_indexer_size(xlw->nodes_names);
	} else
		return 0;
}

uint32_t xlweighter_check_pos(const struct XLayerWeighter *h, const uint32_t n)
{
	uint32_t a,b,i;

	i = 0;
	if(h && h->n_paths > 0)
	{
		a = 0;
		b = h->n_paths-1;
		i = (b+a)/2;
		
		while(b > a )
		{
			if (n > h->paths_id[i])
				a = i+1;
			if (n < h->paths_id[i])
				b = i ? i-1 : 0;
			if (n == h->paths_id[i])
				a = b = i;
		
			i = (b+a)/2;
		}
		if (n > (h->paths_id[i]))
			i++;
	}

	return i;	
}

double xlweighter_peer_weight(const struct XLayerWeighter * xlw,const struct peer * p,const struct nodeID * me)
{
	char ipaddr[80];
	uint32_t src=0,dst=0,path_pos,path_id;
	struct sparse_vector * b,*a;
	double res = -1;
	
	if(xlw && p && me)
	{
		node_ip(me,ipaddr,80);
		if(string_indexer_check(xlw->nodes_names,ipaddr))
		{
			src = string_indexer_id(xlw->nodes_names,ipaddr);
			node_ip(p->id,ipaddr,80);
			if(string_indexer_check(xlw->nodes_names,ipaddr))
			{
				dst = string_indexer_id(xlw->nodes_names,ipaddr);
				if (dst !=src)
				{
					b = sparse_vector_new(0);
					path_id = triel(xlw->nodes_num,src,dst);
					path_pos = xlweighter_check_pos(xlw,path_id);
					a = int_bucket_2_sparse_vector((xlw->paths)[path_pos]);
					sparse_vector_sum(b,a);

				        if(!(xlw->hopcount_only))
						sparse_vector_sum(b,xlw->paths_sum);

					res = sparse_vector_norm(b);

					sparse_vector_destroy(&a);
					sparse_vector_destroy(&b);
				} else
					res = 0; // we are comparing two peers on the same host
			}
		} 
	}
	
	return res;
}

double xlweighter_base_nodes(struct XLayerWeighter * xlw,const struct peerset * pset,const struct nodeID * me)
{
	const struct peer * p1, *p2;
	uint32_t n1,n2,path_pos;
	int i,j;
	char ipaddr[80];
	double c;
	struct int_bucket * sum;

	if(xlw && pset)
	{
		if(xlw->paths_sum)
			sparse_vector_destroy(&(xlw->paths_sum));

		sum = int_bucket_new(0);

		peerset_for_each(pset,p1,i)
		{
			node_ip(p1->id,ipaddr,80);
			if(string_indexer_check(xlw->nodes_names,ipaddr))
			{
				n1 = string_indexer_id(xlw->nodes_names,ipaddr);
				peerset_for_each(pset,p2,j)
				{
					node_ip(p2->id,ipaddr,80);
					if(string_indexer_check(xlw->nodes_names,ipaddr))
					{
						n2 = string_indexer_id(xlw->nodes_names,ipaddr);
						if(n1 < n2)
						{
							path_pos = xlweighter_check_pos(xlw,triel(xlw->nodes_num,n1,n2));
							int_bucket_sum(sum,xlw->paths[path_pos]);
						}
					} else
						fprintf(stderr,"[WARNING] unknown peer\n");
				}
			} else
				fprintf(stderr,"[WARNING] unknown peer\n");
		}

		if(me)
		{
			node_ip(me,ipaddr,80);
			if(peerset_check(pset,me) < 0 && string_indexer_check(xlw->nodes_names,ipaddr))
			{
				n1 = string_indexer_id(xlw->nodes_names,ipaddr);
				peerset_for_each(pset,p2,j)
				{
					node_ip(p2->id,ipaddr,80);
					if(string_indexer_check(xlw->nodes_names,ipaddr))
					{
						n2 = string_indexer_id(xlw->nodes_names,ipaddr);
						if(n1 < n2)
						{
							path_pos = xlweighter_check_pos(xlw,triel(xlw->nodes_num,n1,n2));
							int_bucket_sum(sum,xlw->paths[path_pos]);
						}
					} else
						fprintf(stderr,"[WARNING] unknown peer\n");
				}
			}
		}

		xlw->paths_sum = int_bucket_2_sparse_vector(sum);
		if(xlw->expected_degree)
		{
			n1 = peerset_size(pset) + 1;
			c =((double)((xlw->expected_degree)*n1)-2)/(n1*(n1-1));
			sparse_vector_multiply(xlw->paths_sum,c);
		}
		int_bucket_destroy(&sum);

//		fprintf(stderr,"[INFO] norm_value %f\n",sparse_vector_norm(xlw->paths_sum));
		return sparse_vector_norm(xlw->paths_sum);
	} else
		return -1;
}

int xlweighter_add_path(struct XLayerWeighter * xlw,uint32_t id,struct int_bucket* path)
{
	uint32_t i;

  if(xlw)
  {
  	i = xlweighter_check_pos(xlw,id);
 		if(i>= xlw->n_paths || xlw->paths_id[i] != id)
 		{
 			if((xlw->n_paths + 1) >= xlw->size)
 			{
 				xlw->size += INT_BUCKET_INC_SIZE;
 				xlw->paths = (struct int_bucket * *) realloc(xlw->paths,sizeof(struct int_bucket *) * xlw->size);
 				xlw->paths_id = (uint32_t *) realloc(xlw->paths_id,sizeof(uint32_t) * xlw->size);
 			}
 			memmove(&(xlw->paths[i+1]),&(xlw->paths[i]),sizeof(struct int_bucket *) * (xlw->n_paths -i));
 			memmove(&(xlw->paths_id[i+1]),&(xlw->paths_id[i]),sizeof(uint32_t) * (xlw->n_paths -i));

 			xlw->paths[i] = path;
 			xlw->paths_id[i] = id;
 			xlw->n_paths++;
 		} 
 		return 0;
 	}
 	return -1;
}

int xlweighter_parse_path(struct XLayerWeighter * xlw,char * path_s)
	//input: 10.0.1.1,2,10.0.2.3,1,10.0.5.3
{
	struct int_bucket * path;
	char ** tokens;
	uint32_t i,tok_num, src, dst;

	tokens = tokens_create(path_s,',',&tok_num);
	if(tokens)
	{
		src = string_indexer_id(xlw->nodes_names,tokens[0]);
		dst = string_indexer_id(xlw->nodes_names,tokens[tok_num-1]);
		path = int_bucket_new(tok_num/2);
		for (i = 0;i < tok_num-1;i+=2)
			int_bucket_insert(path,triel(xlw->nodes_num,
				string_indexer_id(xlw->nodes_names,tokens[i]),
				string_indexer_id(xlw->nodes_names,tokens[i+2])),
				atoi(tokens[i+1]));
		tokens_destroy(&tokens,tok_num);
		return xlweighter_add_path(xlw,triel(xlw->nodes_num,src,dst),path);
	} else
		return -1;
}

char * my_strndup(const char * s,uint32_t n)
{
	uint32_t i=0;
	char * ns = NULL;

	if(s)
	{
		ns = (char *) malloc (sizeof(char ) * n);
		while(i<n && s[i] != '\0')
			ns[i] = s[i];
	}

	return ns;
}

int xlweighter_init(struct XLayerWeighter* xlw,const char * path_filename,const char * config)
{
	FILE * fp;
	int res = 0;
	char path_line[MAX_PATH_STRING_LENGTH];
	struct tag *cfg_tags = NULL;

	xlw->n_paths = 0;
	xlw->paths = NULL;
	xlw->paths_id = NULL;
	xlw->paths_sum = sparse_vector_new(0);
	xlw->nodes_num = 0;
	xlw->size = 0;
	xlw->hopcount_only = 0;
	xlw->expected_degree = 0;
  
  if(config && (cfg_tags = grapes_config_parse(config)))
  {
    grapes_config_value_int(cfg_tags,"hopcount",&(xlw->hopcount_only));
    grapes_config_value_int(cfg_tags,"expected_degree",&(xlw->expected_degree));
    free(cfg_tags);
  }

	xlw->nodes_names = string_indexer_new(0);

	fp = fopen(path_filename,"r");
	if(fp != NULL)
	{
		xlw->nodes_num = xlweighter_parse_names(xlw,fp);
		rewind(fp);
		while( fgets(path_line,MAX_PATH_STRING_LENGTH,fp) != NULL)
		{
//			fprintf(stderr,"[DEBUG] read line %s\n",path_line);
			xlweighter_parse_path(xlw,path_line);
		}
		fclose(fp);
	}
	else
	{
		fprintf(stderr,"[ERROR] cannot open %s\n",path_filename);
		res = -1;
	}

	return res;
}

struct XLayerWeighter * xlweighter_new(const char * path_filename)
{
	struct XLayerWeighter * xlw;
  char *c = NULL;

	xlw = (struct XLayerWeighter *)malloc(sizeof(struct XLayerWeighter));
  if(path_filename && (c = strchr(path_filename,',')))
    *(c++) = 0;

	if(xlweighter_init(xlw,path_filename,c) < 0)
		xlweighter_destroy(&xlw);
	return xlw;
}

void xlweighter_destroy(struct XLayerWeighter ** xlw)
{
	uint32_t i;
	struct sparse_vector * v;
	struct int_bucket * ib;
	struct string_indexer * si;

	if(*xlw)
	{
		v = (*xlw)->paths_sum;
		if(v)
			sparse_vector_destroy(&v);

		si =(*xlw)->nodes_names;
		if(si)
			string_indexer_destroy(&si);

		if((*xlw)->paths_id)
			free((*xlw)->paths_id);

		if((*xlw)->paths)
		{
			for( i = 0; i < (*xlw)->n_paths ; i++)
			{
				ib = (*xlw)->paths[i];
				if(ib)
					int_bucket_destroy(&ib);
			}
			free((*xlw)->paths);
		}
		free(*xlw);
		*xlw = NULL;
	}
}



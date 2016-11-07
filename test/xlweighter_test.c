#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <net_helper.h>
#include <peer.h>
#include <peerset.h>
#include "xlweighter.h"

void xlweighter_new_test()
{
	struct XLayerWeighter * xlw;
	FILE * fp;
	char * paths_file= strdup("shortest_paths_test");

	assert(xlweighter_new(NULL) == NULL);
	assert(xlweighter_new("non_existing_file") == NULL);

	fp = fopen(paths_file,"w");
	fputs("10.0.1.1,1,10.0.2.1,1,10.0.3.1\n",fp);
	fputs("10.0.3.1,1,10.0.2.1\n",fp);
	fputs("10.0.1.1,1,10.0.2.1\n",fp);
	fclose(fp);

	xlw = xlweighter_new(paths_file);
	assert(xlw !=NULL);

	xlweighter_destroy(&xlw);
	assert(xlw == NULL);


	free(paths_file);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void xlweighter_base_nodes_test()
{
	FILE * fp;
	char * paths_file = "shortest_paths_test";
	struct nodeID *me,*n1,*n2;
	struct peerset * pset;
	struct XLayerWeighter * xlw;

	fp = fopen(paths_file,"w");
	fputs("10.0.1.1,1,10.0.2.1,1,10.0.3.1\n",fp);
	fputs("10.0.3.1,1,10.0.2.1\n",fp);
	fputs("10.0.1.1,1,10.0.2.1\n",fp);
	fclose(fp);

	me = create_node("10.0.1.1",6666); 
	n1 = create_node("10.0.2.1",6667); 
	n2 = create_node("10.0.3.1",6668); 

	pset = peerset_init(0);
	peerset_add_peer(pset,n1);
	peerset_add_peer(pset,n2);

	xlw = xlweighter_new(paths_file);

	assert(-1 == xlweighter_base_nodes(NULL,NULL,NULL));
	assert(-1 == xlweighter_base_nodes(xlw,NULL,NULL));
	assert(-1 == xlweighter_base_nodes(xlw,NULL,me));
	assert(1 == xlweighter_base_nodes(xlw,pset,NULL));
	assert(xlweighter_base_nodes(xlw,pset,me) > 2.82);
	assert(xlweighter_base_nodes(xlw,pset,me) < 2.83);
	

	peerset_destroy(&pset);
	nodeid_free(me);
	nodeid_free(n1);
	nodeid_free(n2);
	xlweighter_destroy(&xlw);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void xlweighter_peer_weight_test()
{
	FILE * fp;
	char * paths_file = "shortest_paths_test";
	struct nodeID *me,*n1,*n2;
	struct peerset * pset;
	struct XLayerWeighter * xlw;
	struct peer *p;

	fp = fopen(paths_file,"w");
	fputs("10.0.1.1,1,10.0.2.1,1,10.0.3.1\n",fp);
	fputs("10.0.3.1,1,10.0.2.1\n",fp);
	fputs("10.0.1.1,1,10.0.2.1\n",fp);
	fclose(fp);

	me = create_node("10.0.1.1",6666); 
	n1 = create_node("10.0.2.1",6667); 
	n2 = create_node("10.0.3.1",6668); 

	pset = peerset_init(0);
	peerset_add_peer(pset,n1);
	peerset_add_peer(pset,n2);

	xlw = xlweighter_new(paths_file);
	xlweighter_base_nodes(xlw,pset,me);

	assert(xlweighter_peer_weight(NULL,NULL,NULL) < 0 );
	assert(xlweighter_peer_weight(xlw,NULL,NULL) < 0 );
	assert(xlweighter_peer_weight(xlw,NULL,me) < 0 );

	p = peerset_get_peer(pset,n1);
	assert(xlweighter_peer_weight(xlw,p,NULL) < 0 );

	assert(xlweighter_peer_weight(xlw,p,me) > 3.6 );
	assert(xlweighter_peer_weight(xlw,p,me) < 3.61 );

	p = peerset_get_peer(pset,n2);
	assert(xlweighter_peer_weight(xlw,p,me) > 4.24 );
	assert(xlweighter_peer_weight(xlw,p,me) < 4.25 );

	xlweighter_destroy(&xlw);

	paths_file = strdup("shortest_paths_test,hopcount=1");
	xlw = xlweighter_new(paths_file);
	p = peerset_get_peer(pset,n1);
	assert(xlweighter_peer_weight(xlw,p,me) == 1 );
	p = peerset_get_peer(pset,n2);
	assert(xlweighter_peer_weight(xlw,p,me) > 1.4 );
	assert(xlweighter_peer_weight(xlw,p,me) < 1.42 );

	xlweighter_destroy(&xlw);
	free(paths_file);

	paths_file = strdup("shortest_paths_test,expected_degree=2");
	xlw = xlweighter_new(paths_file);
	xlweighter_base_nodes(xlw,pset,me);
	p = peerset_get_peer(pset,n1);
	assert(xlweighter_peer_weight(xlw,p,me) > 2.6 );
//	fprintf(stderr,"value %f\n", xlweighter_peer_weight(xlw,p,me));
	assert(xlweighter_peer_weight(xlw,p,me) < 2.7 );
	p = peerset_get_peer(pset,n2);
	assert(xlweighter_peer_weight(xlw,p,me) > 3.2 );
	assert(xlweighter_peer_weight(xlw,p,me) < 3.3 );

	xlweighter_destroy(&xlw);
	free(paths_file);

	peerset_destroy(&pset);
	nodeid_free(me);
	nodeid_free(n1);
	nodeid_free(n2);


	fprintf(stderr,"%s successfully passed!\n",__func__);
}

int main(char **argc, int argv)
{
	xlweighter_new_test();
	xlweighter_base_nodes_test();
	xlweighter_peer_weight_test();
	return 0;
}

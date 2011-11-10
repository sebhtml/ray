#include <stdlib.h>
#include <routing/ConnectionGraph.h>

int main(int argc,char**argv){

	int n=atoi(argv[1]);

	ConnectionGraph graph;

	graph.buildGraph(n,"random",8,true);

	graph.writeFiles("./");

	return 0;
}

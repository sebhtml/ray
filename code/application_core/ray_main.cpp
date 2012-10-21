/*
 	Ray
    Copyright (C) 2010, 2011, 2012  Sébastien Boisvert

	http://DeNovoAssembler.SourceForge.Net/

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You have received a copy of the GNU General Public License
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/

#include <iostream>
#include "application_core/Machine.h"
#include <stdlib.h>
#include "core/RankProcess.h"
#include <string.h>

using namespace std;

int main(int argc,char**argv){

	int miniRanksPerRank=1;
	int match=0;

/*
 * Get the number of mini-ranks per rank.
 */
	for(int i=0;i<argc;i++){
		if(strcmp(argv[i],"-mini-ranks-per-rank")==match && i+1<argc)
			miniRanksPerRank=atoi(argv[i+1]);
	}

	if(miniRanksPerRank<1)
		miniRanksPerRank=1;

	RankProcess rankProcess;
	rankProcess.constructor(miniRanksPerRank,&argc,&argv);
	
	int rank=rankProcess.getMessagesHandler()->getRank();
	int size=rankProcess.getMessagesHandler()->getSize();

	cout<<"Will use "<<miniRanksPerRank<<" mini-ranks per rank"<<endl;

	int totalMiniranks=size*miniRanksPerRank;

/*
 * Add the mini-ranks.
 */
	for(int i=0;i<miniRanksPerRank;i++){
		int miniRankNumber=rank*miniRanksPerRank+i;
		
		cout<<"Building MiniRank object"<<endl;

		MiniRank*miniRank=new Machine(argc,argv,miniRankNumber,totalMiniranks);

		rankProcess.addMiniRank(miniRank);
	}

	rankProcess.run();

	return EXIT_SUCCESS;
}


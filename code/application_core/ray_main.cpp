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
#include "core/VirtualMachine.h"

using namespace std;

int main(int argc,char**argv){

	int miniRanksPerRank=4;

	VirtualMachine virtualMachine;
	virtualMachine.constructor(miniRanksPerRank);
	
	int rank=virtualMachine.getRank();
	int size=virtualMachine.getSize();

	int totalMiniranks=size*miniRanksPerRank;

	for(int i=0;i<miniRanksPerRank;i++){
		int miniRank=rank*miniRanksPerRank+i;
		Machine*m=new Machine(argc,argv,miniRank,totalMiniranks);

		virtualMachine->addMiniRank(m);
	}

	virtualMachine->run();

	return EXIT_SUCCESS;
}


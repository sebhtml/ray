/*
 	Ray
    Copyright (C) 2010  Sébastien Boisvert

	http://DeNovoAssembler.SourceForge.Net/

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You have received a copy of the GNU General Public License
    along with this program (COPYING).  
	see <http://www.gnu.org/licenses/>

*/


#include<ErrorSimulator.h>
#include<iostream>
using namespace std;

int main(int argc,char**argv){
	if(argc!=3){
		cout<<"Usage"<<endl;
		cout<<argv[0]<<" input.fasta output.fasta"<<endl;
		return 0;
	}
	ErrorSimulator simulator;
	simulator.simulateErrors(argv[1],argv[2]);
	return 0;
}

/*
 	OpenAssembler -- a de Bruijn DNA assembler for mixed high-throughput technologies
    Copyright (C) 2009  Sébastien Boisvert

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


#ifndef _ErrorSimulator
#define _ErrorSimulator

#include<string>
using namespace std;

class ErrorSimulator{
public:
	void simulateErrors(string inputFile,string outputFile);
	void simulateInsertion(string*a,int i);
	void simulateMismatch(string *a,int i);
	void simulateDeletion(string*a,int i);
	void simulateUnknownBase(string*a,int i);
};

#endif

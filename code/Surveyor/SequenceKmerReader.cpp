/*
    Copyright 2013 Maxime Déraspe
    Copyright 2013 Université Laval

    This file is part of Ray Surveyor.

    Ray Surveyor is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.

    Ray Surveyor is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Ray Surveyor.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "SequenceKmerReader.h"

#include <iostream>
#include <sstream>
#include <map>
using namespace std;

#include <string.h>

SequenceKmerReader::SequenceKmerReader(){
}

SequenceKmerReader::~SequenceKmerReader(){
}

void SequenceKmerReader::openFile(string & fileName, int kmerSize){
       
        m_kmerSize = kmerSize;

	m_reader.open(fileName.c_str());

	m_bad = false;

	if(!m_reader.isValid())
		m_bad = true;

	m_loaded = 0;

	cout <<"opens file " << fileName << endl;
        
        m_hasKmerLeft = true;
}


bool SequenceKmerReader::hasAnotherKmer(){
        return m_hasKmerLeft;
}


void SequenceKmerReader::fetchNextKmer(string & kmer){

	m_buffer[0] = '\0';

        bool endOfFile = m_reader.eof();

        ofstream kmerFile;

        while(m_tmpSequence.length() < (unsigned) m_kmerSize && !endOfFile){
                m_reader.getline(m_buffer, 1024);
                if (m_buffer[0] != '>'){
                        m_buffer[strlen(m_buffer)-1] = '\0';
                        m_tmpSequence += m_buffer;
                }
        }

        if(m_tmpSequence.length() >= (unsigned) m_kmerSize){
                
                kmerFile.open("kmers.out.txt",ios::app);
                kmerFile << m_tmpSequence.substr(0,m_kmerSize-1) << "\n";
                kmerFile.close();

                kmer = m_tmpSequence.substr(0,m_kmerSize-1);
                convertSequenceToUpperCase(kmer);
                m_tmpSequence.erase(0,1);
        }

        m_hasKmerLeft = (!endOfFile || m_tmpSequence.length() >= (unsigned) m_kmerSize) ;

}



void SequenceKmerReader::convertSequenceToUpperCase(string & sequence) {
        ///////////////////////////////////////////////////////////////////////
        // convert the sequence to upper case

        map<char,char> translationTable;
        translationTable['a'] = 'A';
        translationTable['t'] = 'T';
        translationTable['g'] = 'G';
        translationTable['c'] = 'C';

        for(int i = 0 ; i < (int) sequence.length() ; ++i) {

                char symbol = sequence[i];

                if(translationTable.count(symbol) > 0) {
                        char correct = translationTable[symbol];

                        sequence[i] = correct;
                }
        }
#if 0
        cout << "DEBUG " << sequence << " with " << coverage << endl;
#endif


}

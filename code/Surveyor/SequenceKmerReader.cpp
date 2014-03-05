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

        m_emptyBuffer = false;
        
        m_kmerSize = kmerSize;

	m_reader.open(fileName.c_str());

	m_bad = false;

	if(!m_reader.isValid())
		m_bad = true;

	m_loaded = 0;

        m_hasKmerLeft = true;
}


bool SequenceKmerReader::hasAnotherKmer(){
        return m_hasKmerLeft;
}


void SequenceKmerReader::fetchNextKmer(string & kmer){

        if (strlen(m_buffer) == 0){
                m_buffer[0] = '\0';
        }else{
                addBufferToTmpSequence();                
        }

        m_hasKmerLeft = !(m_tmpSequence.length() < (unsigned) m_kmerSize && m_reader.eof());

        while(m_tmpSequence.length() < (unsigned) m_kmerSize && !m_reader.eof()){
                m_reader.getline(m_buffer, 1024);
                if (m_buffer[0] != '>'){

                        if(m_buffer[strlen(m_buffer)-1] == '\n'){
                                m_buffer[strlen(m_buffer)-1] = '\0';
                        }
                        addBufferToTmpSequence();
                }
        }

        if(m_tmpSequence.length() >= (unsigned) m_kmerSize){
                kmer = m_tmpSequence.substr(0,m_kmerSize);
                convertSequenceToUpperCase(kmer);
                m_tmpSequence.erase(0,1);
        }

}


void SequenceKmerReader::addBufferToTmpSequence() {

        bool untilEndOfBuffer = true;

        int i = 0;

        while (m_buffer[i] != '\0'  && untilEndOfBuffer == true){
                if(m_buffer[i] == 'N' || m_buffer[i] == 'n' ){
                        if(m_tmpSequence.length() < (unsigned) m_kmerSize){
                                m_tmpSequence.clear();
                                if (m_reader.eof() && 
                                    (strlen(m_buffer) - i + m_tmpSequence.length()) < (unsigned) m_kmerSize){
                                        untilEndOfBuffer = false;
                                }
                        }
                        else {
                                m_buffer[0] = m_buffer[i];
                                untilEndOfBuffer = false;
                        }
                } else {
                        m_tmpSequence += m_buffer[i];
                }
                ++i;
        }

        if (untilEndOfBuffer) {
                m_buffer[0] = '\0';
        }

        if (m_reader.eof()) {
                if (m_tmpSequence.length() + strlen(m_buffer) < (unsigned) m_kmerSize){
                        m_hasKmerLeft = false;
                }
        }

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

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


#ifndef SequenceKmerReaderHeader
#define SequenceKmerReaderHeader

#include <string>
using namespace std;

#include <RayPlatform/files/FileReader.h>

class SequenceKmerReader {


private:

        int m_kmerSize;
        string m_tmpSequence;
        bool m_hasKmerLeft;
        bool m_emptyBuffer;

        // fast IO using a wrapper.
        FileReader m_reader;
        char m_buffer[1024];
        bool m_bad;
        int m_loaded;

        void convertSequenceToUpperCase(string & sequence);
        void addBufferToTmpSequence();

public:

        SequenceKmerReader();
        ~SequenceKmerReader();

        void openFile(string & fileName, int kmerSize);
        bool hasAnotherKmer();
        void fetchNextKmer(string & kmer);

};



#endif

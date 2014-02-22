/*
 * Copyright 2014 Sébastien Boisvert
 *
 * This file is part of Ray.
 *
 * Ray Surveyor is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * Ray Surveyor is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Ray Surveyor.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "PathMaster.h"

#include <code/Mock/Logger.h>

// #define DEBUG_THE_PATH_MASTER

#include <assert.h>

PathMaster::PathMaster(){

}

PathMaster::~PathMaster () {

}

void PathMaster::initialize(Parameters * parameters) {

	m_parameters = parameters;
}

void PathMaster::compare(GraphPath & path1, bool strand1, int &bestLast1,
	GraphPath & path2, bool strand2, int &bestLast2, int &bestMatches) {

	vector<Kmer> kmers;

	path2.getVertices(&kmers);

	// TODO only index a band instead of all the positions
	// the calling code has this information
	map<Kmer, vector<int> > hitKmers;
	for(int i = 0 ; i < (int) kmers.size() ; ++i) {

		int position = i;
		Kmer kmer = kmers[position];

		hitKmers[kmer].push_back(position);
	}

	int matches = 0;
	int noValue = -1;
	int previous = noValue;


	for(int i = 0 ; i < (int) path1.size() ; ++i) {

		Kmer kmer;
		path1.at(i,&kmer);

		if(strand1 == true) {


			kmer = kmer.complementVertex(m_parameters->getWordSize(),
				m_parameters->getColorSpaceMode());
		}

		// is it in path2 ?

		if(hitKmers.count(kmer) > 0) {

			vector<int> & positions = hitKmers[kmer];

			for(int j = 0 ; j < (int) positions.size() ; ++j) {

				int position = positions[j];

				// it is the first time we see this path
				if(previous == noValue) {

					previous = position;
					matches = 0;

					continue;
				}

				// continue the threading
				if((!strand1 && position == previous + 1 )
					       || (strand1 && position == previous - 1)) {

					if(matches == 0)
						matches = 1; // for the first kmer

					matches++;

					previous = position;
				} else {

					previous = noValue;
				}

				// check the score.
				if(matches > bestMatches) {

					bestMatches = matches;

					if(strand1) {

						/*                             i
						 *                             |
						 * ---------------------------->
						 * |||||||||||||||||||||||||||||
						 * <----------------------------
						 *                    -------------------------->
						 *                             |
						 *                             position
						 *
						 *
						 */

						/*
						bestLast1 = path1.size() - 1 - i + bestMatches - 1;
						bestLast2 = position + bestMatches - 1;
						*/

						bestLast1 =  path1.size() -1 -i + bestMatches -1;
						bestLast2 = position + bestMatches -1;
					} else {

						/*                        i
						 *                        |
						 *     ------------------->
						 *                -------------------->
						 *                        |
						 *                        position
						 *
						 */
						bestLast1 = i;
						bestLast2 = position;
					}


				}

			}
		}
	}
}


/*
 *                                  bestLast1
 *                                    |
 * ------------------------------------->
 *
 *                           ----------------------------->
 *                                    |
 *                                   bestLast2
 */
void PathMaster::combine(GraphPath & newPath, GraphPath & path1, bool strand1, int bestLast1,
		GraphPath & path2, bool strand2, int bestLast2) {

	newPath.setKmerLength(m_parameters->getWordSize());


	assert(bestLast1 >= 0);
	assert(bestLast1 < (int)path1.size());
	assert(bestLast2 >= 0);
	assert(bestLast2 < (int) path2.size());

#ifdef DEBUG_THE_PATH_MASTER
	cout << "DEBUG combine path1 pathSize " << path1.size() << " strand " << strand1 << " position " << bestLast1 << endl;
	display(path1, strand1, bestLast1-30, bestLast1 + 30, bestLast1);

	cout << "DEBUG pathSize path2 " << path2.size() << " strand " << strand2 << " position " << bestLast2 << endl;
	display(path2, strand2, bestLast2-30, bestLast2 + 30, bestLast2);

#endif

	LOG(INFO) << "DEBUG length newPath (t=  0) " << newPath.size() << endl;

	/* we take directly the path */
	if(!strand1){

		for(int i = 0 ; i <= (int) bestLast1; ++i) {
			Kmer kmer;
			path1.at(i, &kmer);
			newPath.push_back(&kmer);
		}

	}else{
		/* we need the reverse complement path */
		GraphPath rc;
		path1.reverseContent(rc);

		for(int i = 0 ; i <= (int) bestLast1; ++i) {
			Kmer kmer;
			rc.at(i, &kmer);
			newPath.push_back(&kmer);
		}

	}

	LOG(INFO) << "DEBUG length newPath (t=  1) " << newPath.size() << endl;

	bool hasError = false;

	GraphPath rc;

	GraphPath * selection = &path2;

	if(strand2) {
		path2.reverseContent(rc);
		selection = &rc;
	}

	/* other path is always forward strand */
	for(int i= bestLast2 + 1 ;i<(int)selection->size() ; ++i){
		Kmer otherKmer;
		selection->at(i,&otherKmer);

		int before = newPath.size();

		newPath.push_back(&otherKmer);

		int after = newPath.size();

		// there was an error here.
		if(before == after && !hasError) {

			hasError = true;

			LOG(WARNING) << "Warning: problem detected while creating new path." << endl;
		}
	}


	LOG(INFO) << "DEBUG length newPath (t=  2) " << newPath.size() << endl;

}

void PathMaster::display(GraphPath & path, bool strand, int start, int end, int special) {

	int kmerLength = m_parameters->getWordSize();
	if(start < 0)
		start = 0;

	if(end > (int)path.size() -1) {
		end = path.size() -1;
	}


	GraphPath rc;


	GraphPath * selection = &path;

	if(strand) {

		path.reverseContent(rc);

		selection = &rc;
	}

	for(int i = start ; i <= end ; ++i) {

		Kmer theObject;
		selection->at(i, &theObject);


		cout<<" Position: "<< i << " Strand: " << strand;
		cout << " Sequence: " << theObject.idToWord(kmerLength,false);

		if(i == special) {
			cout << " <--------------- GENETIC MARKER";
		}

		cout << endl;

	}


}

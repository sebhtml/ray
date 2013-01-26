/*
    Ray -- Parallel genome assemblies for parallel DNA sequencing
    Copyright (C) 2010, 2011, 2012, 2013 Sébastien Boisvert

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

#include "GraphPath.h"

#include <code/plugin_Mock/constants.h>
#include <code/plugin_Mock/common_functions.h>

#include <RayPlatform/core/statistics.h>

#include <iostream>
#include <map>
using namespace std;

#include <string.h>

//#define CONFIG_PATH_VERBOSITY
//#define CHECK_BUG_142

GraphPath::GraphPath(){
	m_hasPeakCoverage=false;

	m_kmerLength=0;
	m_errorRaised=false;

#ifdef CONFIG_PATH_STORAGE_BLOCK
	m_size=0;
#endif
}

int GraphPath::size()const{

#ifdef CONFIG_PATH_STORAGE_DEFAULT
	return m_vertices.size();
#elif defined(CONFIG_PATH_STORAGE_BLOCK)
	return m_size;
#endif

}

void GraphPath::at(int i,Kmer*value)const{

#ifdef CONFIG_PATH_STORAGE_DEFAULT
	(*value)=m_vertices.at(i);
#elif defined(CONFIG_PATH_STORAGE_BLOCK)

	readObjectInBlock(i,value);
#endif
}

CoverageDepth GraphPath::getCoverageAt(int position)const{

	if(m_coverageValues.size()==0)
		return 0;

	return m_coverageValues[position];
}

bool GraphPath::canBeAdded(const Kmer*object)const{

	if(size()==0)
		return true;

	Kmer lastKmer;
	int position=size()-1;
	at(position,&lastKmer);

	return lastKmer.canHaveChild(object,m_kmerLength);
}

void GraphPath::push_back(const Kmer*a){

#ifdef ASSERT
	assert(m_kmerLength!=0);
#endif

	if(!canBeAdded(a)){
		if(!m_errorRaised){
			cout<<"Error: can not add "<<a->idToWord(m_kmerLength,false)<<endl;
			cout<<"last objects:"<<endl;
			int count=16;
			int iterator=size()-count;
			while(iterator<size()){
				Kmer theObject;
				at(iterator,&theObject);

				cout<<" ["<<iterator<<"] ------> "<<theObject.idToWord(m_kmerLength,false)<<endl;

				iterator++;
			}

			m_errorRaised=true;
		}

		return;
	}

#ifdef CONFIG_PATH_STORAGE_DEFAULT
	m_vertices.push_back(*a);
#elif defined(CONFIG_PATH_STORAGE_BLOCK)

	writeObjectInBlock(a);
#endif
}

void GraphPath::getVertices(vector<Kmer>*vertices)const{
	for(int i=0;i<size();i++){
		Kmer kmer;
		at(i,&kmer);
		vertices->push_back(kmer);
	}
}

void GraphPath::clear(){
#ifdef CONFIG_PATH_STORAGE_DEFAULT
	m_vertices.clear();
#elif defined(CONFIG_PATH_STORAGE_BLOCK)
	m_blocks.clear();
	m_size=0;
	m_kmerLength=0;
#endif
}

void GraphPath::resetCoverageValues(){
	m_coverageValues.clear();
}

void GraphPath::computePeakCoverage(){

	int ALGORITHM_MODE=0;
	int ALGORITHM_MEAN=1;
	int ALGORITHM_STAGGERED_MEAN=2;

	int selectedAlgorithm=ALGORITHM_STAGGERED_MEAN;

	#ifdef ASSERT
	assert((int)m_coverageValues.size() == size());
	#endif

	// the default is to use the weighted mean algorithm 

	if(selectedAlgorithm==ALGORITHM_MODE){
		computePeakCoverageUsingMode();
	}else if(selectedAlgorithm==ALGORITHM_MEAN){
		computePeakCoverageUsingMean();
	}else if(selectedAlgorithm==ALGORITHM_STAGGERED_MEAN){
		computePeakCoverageUsingStaggeredMean();
	}

	m_hasPeakCoverage=true;
}

CoverageDepth GraphPath::getPeakCoverage()const{

	#ifdef ASSERT
	assert(m_hasPeakCoverage == true);
	#endif
	
	return m_peakCoverage;
}

void GraphPath::addCoverageValue(CoverageDepth value){
	m_coverageValues.push_back(value);
}

void GraphPath::computePeakCoverageUsingMode(){

	map<CoverageDepth,int> frequencies;

	for(int i=0;i<(int)m_coverageValues.size();i++){
		frequencies[m_coverageValues[i]]++;
	}

	int best=-1;

	for(map<CoverageDepth,int>::iterator i=frequencies.begin();
		i!=frequencies.end();i++){

		if(frequencies.count(best)==0 || i->second > frequencies[best]){
			best=i->first;
		}

		#ifdef CONFIG_VERBOSITY_FOR_SEEDS
		cout<<i->first<<"	"<<i->second<<endl;
		#endif
	}

	#ifdef CONFIG_VERBOSITY_FOR_SEEDS
	cout<<"mode= "<<best<<" length= "<<size()<<endl;
	#endif

	m_peakCoverage=best;

}

void GraphPath::computePeakCoverageUsingMean(){

	map<CoverageDepth,int> frequencies;

	for(int i=0;i<(int)m_coverageValues.size();i++){
		frequencies[m_coverageValues[i]]++;
	}

	LargeCount sum=0;
	LargeCount count=0;

	for(map<CoverageDepth,int>::iterator i=frequencies.begin();
		i!=frequencies.end();i++){

		CoverageDepth coverage=i->first;
		LargeCount frequency=i->second;

		#ifdef CONFIG_VERBOSITY_FOR_SEEDS
		cout<<coverage<<"	"<<frequency<<endl;
		#endif

		sum+=coverage*frequency;
		count+=frequency;
	}

	#ifdef ASSERT
	assert(m_coverageValues.size()>=1);
	assert(count!=0);
	assert(count>0);
	assert(sum > 0);
	#endif

	CoverageDepth mean=( sum / count );

	cout<<"mean= "<<mean <<" length= "<<size()<<endl;

	m_peakCoverage=mean;
}

void GraphPath::reserve(int size){
#ifdef CONFIG_PATH_STORAGE_DEFAULT
	m_vertices.reserve(size);
#endif
	m_coverageValues.reserve(size);

}

void GraphPath::computePeakCoverageUsingStaggeredMean(){

	map<CoverageDepth,int> frequencies;
	uint64_t totalCount=0;

	for(int i=0;i<(int)m_coverageValues.size();i++){
		frequencies[m_coverageValues[i]]++;

		totalCount++;
	}

	CoverageDepth NO_VALUE=0;

	CoverageDepth mean=NO_VALUE;
	uint64_t objectsOnRight=0;

/*
 * 10% is arbitrary, anything between 5% and 45% will be fine.
 */
	int thresholdInPercentage=10;
	uint64_t minimumRequired=thresholdInPercentage*totalCount/100;

	while(1){
		LargeCount sum=0;
		LargeCount count=0;

		for(map<CoverageDepth,int>::iterator i=frequencies.begin();
			i!=frequencies.end();i++){

			CoverageDepth coverage=i->first;
			LargeCount frequency=i->second;

			#ifdef CONFIG_VERBOSITY_FOR_SEEDS
			cout<<coverage<<"	"<<frequency<<endl;
			#endif

/*
 * Skip the repeats.
 */
			if(mean!=NO_VALUE && coverage>=mean)
				continue;

			sum+=coverage*frequency;
			count+=frequency;
		}

		#ifdef ASSERT
		assert(m_coverageValues.size()>=1);
		assert(count!=0);
		assert(count>0);
		assert(sum > 0);
		#endif

		mean=( sum / count );

/*
 * Count the number of objects that are at least the mean.
 */

		objectsOnRight=0;

		for(map<CoverageDepth,int>::iterator i=frequencies.begin();
			i!=frequencies.end();i++){

			CoverageDepth coverage=i->first;
			LargeCount frequency=i->second;

			if(coverage>=mean)
				objectsOnRight+=frequency;
		}

		if(objectsOnRight>=minimumRequired){
			break;
		}
	}

	#ifdef CONFIG_VERBOSITY_FOR_SEEDS
	cout<<"mean= "<<mean <<" length= "<<size()<<" onTheRight= "<<objectsOnRight<<"/"<<totalCount<<endl;
	#endif

	m_peakCoverage=mean;
}

void GraphPath::setKmerLength(int kmerLength){
	m_kmerLength=kmerLength;

	#ifdef ASSERT
	assert(kmerLength!=0);
	assert(m_kmerLength!=0);
	#endif
}

#ifdef CONFIG_PATH_STORAGE_BLOCK

void GraphPath::writeObjectInBlock(const Kmer*a){

	#ifdef ASSERT
	assert(m_kmerLength!=0);
	#endif

#ifdef CHECK_BUG_142
	string copyA="AGGAAGAACCTGCTGAGGAACAAGAAGGTCAACTGCCTGGACTGTAATACC";
	string copyB=a->idToWord(m_kmerLength,false);
	if(copyA==copyB)
		cout<<"[GraphPath::writeObjectInBlock] returns "<<copyB<<endl;
#endif

	if(m_size==0){
		#ifdef ASSERT
		assert(m_blocks.size()==0);
		#endif

		GraphPathBlock block;
		m_blocks.push_back(block);
		string sequence=a->idToWord(m_kmerLength,false);
		int blockNumber=0;
		int blockPosition=0;
		memcpy(m_blocks[blockNumber].m_content+blockPosition,sequence.c_str(),m_kmerLength);
	}else{
		#ifdef ASSERT
		assert(m_size>=1);
		assert(a!=NULL);
		assert(m_kmerLength!=0);
		#endif

		char lastSymbol=a->getLastSymbol(m_kmerLength,false);
		int usedSymbols=size()+m_kmerLength-1;

		#ifdef ASSERT
		assert(usedSymbols>=m_kmerLength);
		assert(m_blocks.size()>=1);
		#endif

		int allocatedSymbols=m_blocks.size()*CONFIG_PATH_BLOCK_SIZE;

		#ifdef ASSERT
		assert(allocatedSymbols>=CONFIG_PATH_BLOCK_SIZE);
		#endif

		if(usedSymbols+1>allocatedSymbols){
			GraphPathBlock block;
			m_blocks.push_back(block);
			allocatedSymbols=m_blocks.size()*CONFIG_PATH_BLOCK_SIZE;
		}

		#ifdef ASSERT
		assert(usedSymbols+1<=allocatedSymbols);
		assert(allocatedSymbols>=CONFIG_PATH_BLOCK_SIZE);
		#endif

		int position=usedSymbols;
		int blockNumber=position/CONFIG_PATH_BLOCK_SIZE;
		int positionInBlock=position%CONFIG_PATH_BLOCK_SIZE;

		#ifdef ASSERT
		assert(blockNumber<(int)m_blocks.size());
		assert(positionInBlock<CONFIG_PATH_BLOCK_SIZE);
		assert(blockNumber>=0);
		assert(positionInBlock>=0);
		#endif

		m_blocks[blockNumber].m_content[positionInBlock]=lastSymbol;
	}

	m_size++;

#ifdef ASSERT
	Kmer addedObject;
	at(size()-1,&addedObject);

	if((*a)!=addedObject){
		cout<<"Error: expected: "<<a->idToWord(m_kmerLength,false)<<endl;
		cout<<"actual: "<<addedObject.idToWord(m_kmerLength,false)<<" at position "<<size()-1<<endl;
		cout<<"kmerLength: "<<m_kmerLength<<" CONFIG_PATH_BLOCK_SIZE "<<CONFIG_PATH_BLOCK_SIZE<<endl;
		int i=size()-1;
		int j=0;
		cout<<"dump:"<<endl;
		while(i-j>=0 && j<10){
			Kmer theObject;
			at(i-j,&theObject);

			cout<<" ["<<i-j<<"] ------> "<<theObject.idToWord(m_kmerLength,false)<<endl;

			j++;
		}
	}

	assert((*a)==addedObject);
#endif
}

void GraphPath::readObjectInBlock(int position,Kmer*object)const{
	#ifdef ASSERT
	assert(position<size());
	assert(position>=0);
	assert(m_kmerLength!=0);
	#endif

	int blockNumber=position/CONFIG_PATH_BLOCK_SIZE;
	int positionInBlock=position%CONFIG_PATH_BLOCK_SIZE;

	#ifdef ASSERT
	assert(blockNumber<(int)m_blocks.size());
	assert(positionInBlock<CONFIG_PATH_BLOCK_SIZE);
	#endif

	char kmer[CONFIG_MAXKMERLENGTH+1];

	int lastPosition=position+m_kmerLength-1;

	int blockNumberForLastPosition=lastPosition/CONFIG_PATH_BLOCK_SIZE;
#if 0
	int positionInBlockForLastPosition=lastPosition%CONFIG_PATH_BLOCK_SIZE;
#endif

	if(blockNumber==blockNumberForLastPosition){
		const char*block=m_blocks[blockNumber].m_content;
		int count=m_kmerLength;
		memcpy(kmer,block+positionInBlock,count);

#ifdef CONFIG_PATH_VERBOSITY
		cout<<"[Case1] [1/1] Copying "<<count<<" from block coordinate ("<<blockNumber<<","<<positionInBlock<<")"<<endl;
#endif

	}else{

		#ifdef ASSERT
		assert(blockNumber+1==blockNumberForLastPosition);
		assert(blockNumber+1<(int)m_blocks.size());
		#endif

		const char*block=m_blocks[blockNumber].m_content;
		int count=(CONFIG_PATH_BLOCK_SIZE-1)-positionInBlock+1;
		memcpy(kmer,block+positionInBlock,count);

#ifdef CONFIG_PATH_VERBOSITY
		cout<<"[Case2] [1/2] Copying "<<count<<" from block coordinate ("<<blockNumber<<","<<positionInBlock<<")"<<endl;
#endif

		int blockNumber2=blockNumber+1;
		const char*block2=m_blocks[blockNumber2].m_content;
		int count2=m_kmerLength-count;
		int positionInBlock2=0;
		memcpy(kmer+count,block2+positionInBlock2,count2);

#ifdef CONFIG_PATH_VERBOSITY
		cout<<"[Case2] [2/2] Copying "<<count2<<" from block coordinate ("<<blockNumber2<<","<<positionInBlock2<<")"<<endl;
#endif
	}

	kmer[m_kmerLength]='\0';

#ifdef CHECK_BUG_142
	string copyA="AGGAAGAACCTGCTGAGGAACAAGAAGGTCAACTGCCTGGACTGTAATACC";
	string copyB=kmer;
	if(copyA==copyB){
		cout<<"[GraphPath::readObjectInBlock] returns "<<copyB<<endl;
		cout<<"position: "<<position<<" blocks: "<<m_blocks.size()<<" vertices: "<<size()<<" kmerLength: "<<m_kmerLength<<endl;
		cout<<"blockNumber: "<<blockNumber<<" positionInBlock: "<<positionInBlock<<endl;
		cout<<"CONFIG_PATH_BLOCK_SIZE "<<CONFIG_PATH_BLOCK_SIZE<<endl;
	}
#endif

#ifdef CONFIG_PATH_VERBOSITY
	cout<<"Object: "<<kmer<<endl;
#endif

	(*object)=wordId(kmer);
}

#endif

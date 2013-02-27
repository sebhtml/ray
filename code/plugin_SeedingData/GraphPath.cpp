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
	if(m_coverageValues.size()!=size())
		cout<<"Error: there are "<<size()<<" objects, but only "<<m_coverageValues.size()<<" coverage values"<<endl;

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

		addBlock();
		string sequence=a->idToWord(m_kmerLength,false);

		for(int blockPosition=0;blockPosition<m_kmerLength;blockPosition++){
			writeSymbolInBlock(blockPosition,sequence[blockPosition]);
		}
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

		int allocatedSymbols=m_blocks.size()*getBlockSize();

		#ifdef ASSERT
		assert(allocatedSymbols>=getBlockSize());
		#endif

		if(usedSymbols+1>allocatedSymbols){
			addBlock();
			allocatedSymbols=m_blocks.size()*getBlockSize();
		}

		#ifdef ASSERT
		assert(usedSymbols+1<=allocatedSymbols);
		assert(allocatedSymbols>=getBlockSize());
		#endif

		int position=usedSymbols;

		#ifdef ASSERT
		assert(position<allocatedSymbols);
		#endif

		writeSymbolInBlock(position,lastSymbol);
	}

	m_size++;

#ifdef ASSERT
	Kmer addedObject;
	at(size()-1,&addedObject);

	if((*a)!=addedObject){
		cout<<"Error: expected: "<<a->idToWord(m_kmerLength,false)<<endl;
		cout<<"actual: "<<addedObject.idToWord(m_kmerLength,false)<<" at position "<<size()-1<<endl;
		cout<<"kmerLength: "<<m_kmerLength<<" blockSize: "<<getBlockSize()<<endl;
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

int GraphPath::getBlockSize()const{
	return CONFIG_PATH_BLOCK_SIZE;
}

void GraphPath::readObjectInBlock(int position,Kmer*object)const{

	#ifdef ASSERT
	assert(position<size());
	assert(position>=0);
	assert(m_kmerLength!=0);
	#endif

	char kmer[CONFIG_MAXKMERLENGTH+1];

	for(int i=0;i<m_kmerLength;i++){
		kmer[i]=readSymbolInBlock(position+i);
	}

	kmer[m_kmerLength]='\0';

#ifdef CONFIG_PATH_VERBOSITY
	cout<<"Object: "<<kmer<<endl;
#endif

	(*object)=wordId(kmer);
}

char GraphPath::readSymbolInBlock(int position)const{

	int globalPosition=position*BITS_PER_NUCLEOTIDE;
	int numberOfBitsPerBlock=CONFIG_PATH_BLOCK_SIZE*BITS_PER_NUCLEOTIDE;

/*
 * Example:
 *
 * CONFIG_PATH_BLOCK_SIZE: 4096
 * blocks: 3
 * availableSymbols: 12288
 * total bits: 24576
 * bits per block: 8192
 *
 * position: 9999
 * bit position: 19998
 * block for the bit: 19998/8192 = 2
 * uint64_t in block for the bit: 19998%8192/64 = 3614/64 = 56
 * bit in uint64_t in block: 19998%8192%64 = (19998%8192)%64 = 30
 *
 * The address of position 9999 is (2,56,30).
 */

/* a block contains an array of uint64_t */
	int blockNumber=globalPosition/numberOfBitsPerBlock;

/* this is the index of the uint64_t in the block */
	int positionInBlock=(globalPosition%numberOfBitsPerBlock)/(sizeof(uint64_t)*BITS_PER_BYTE);
	int bitPosition=(globalPosition%numberOfBitsPerBlock)%(sizeof(uint64_t)*BITS_PER_BYTE);

	uint64_t oldChunkValue=m_blocks[blockNumber].m_content[positionInBlock];

	oldChunkValue<<=(sizeof(uint64_t)*BITS_PER_BYTE-BITS_PER_NUCLEOTIDE-bitPosition);
	oldChunkValue>>=(sizeof(uint64_t)*BITS_PER_BYTE-BITS_PER_NUCLEOTIDE);

	uint8_t code=oldChunkValue;

#ifdef ASSERT
	assert(code==RAY_NUCLEOTIDE_A||code==RAY_NUCLEOTIDE_T||code==RAY_NUCLEOTIDE_C||code==RAY_NUCLEOTIDE_G);
#endif

	char symbol=codeToChar(code,false);

	return symbol;
}

void GraphPath::writeSymbolInBlock(int position,char symbol){

	int globalPosition=position*BITS_PER_NUCLEOTIDE;
	int numberOfBitsPerBlock=CONFIG_PATH_BLOCK_SIZE*BITS_PER_NUCLEOTIDE;

/* a block contains an array of uint64_t */
	int blockNumber=globalPosition/numberOfBitsPerBlock;

/* this is the index of the uint64_t in the block */
	int positionInBlock=(globalPosition%numberOfBitsPerBlock)/(sizeof(uint64_t)*BITS_PER_BYTE);
	int bitPosition=(globalPosition%numberOfBitsPerBlock)%(sizeof(uint64_t)*BITS_PER_BYTE);

	uint64_t oldChunkValue=m_blocks[blockNumber].m_content[positionInBlock];

	uint64_t mask=charToCode(symbol);
	mask<<=bitPosition;

	oldChunkValue|=mask;

	m_blocks[blockNumber].m_content[positionInBlock]=oldChunkValue;

#ifdef ASSERT
	if(readSymbolInBlock(position)!=symbol){
		cout<<"Expected "<<symbol<<" Actual "<<readSymbolInBlock(position)<<endl;
	}

	assert(readSymbolInBlock(position)==symbol);
#endif
}

void GraphPath::addBlock(){

	GraphPathBlock block;
	m_blocks.push_back(block);

	for(int i=0;i<(int)NUMBER_OF_64_BIT_INTEGERS;i++){
		m_blocks[m_blocks.size()-1].m_content[i]=0;
	}
}

#endif

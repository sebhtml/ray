/*
 	Ray
    Copyright (C) 2010, 2011  Sébastien Boisvert

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

#include <plugin_SeedExtender/BubbleTool.h>
#include <application_core/common_functions.h>
#include <assert.h>
#include <map>
#include <set>
#include <iostream>
using namespace std;

void BubbleTool::printStuff(Kmer root,vector<vector<Kmer> >*trees,
map<Kmer,int>*coverages){
	int m_wordSize=m_parameters->getWordSize();
	cout<<"Trees="<<trees->size()<<endl;
	cout<<"root="<<root.idToWord(m_wordSize,m_parameters->getColorSpaceMode())<<endl;
	cout<<"digraph{"<<endl;
	map<Kmer,set<Kmer> > printedEdges;
	
	for(map<Kmer ,int>::iterator i=coverages->begin();i!=coverages->end();i++){
		Kmer kmer=i->first;
		cout<<kmer.idToWord(m_wordSize,m_parameters->getColorSpaceMode())<<" [label=\""<<kmer.idToWord(m_wordSize,m_parameters->getColorSpaceMode())<<" "<<i->second<<"\"]"<<endl;
	}
	for(int j=0;j<(int)trees->size();j++){
		for(int i=0;i<(int)trees->at(j).size();i+=2){
			Kmer a=trees->at(j).at(i+0);
			#ifdef ASSERT
			assert(i+1<(int)trees->at(j).size());
			#endif
			Kmer b=trees->at(j).at(i+1);
			if(printedEdges.count(a)>0 && printedEdges[a].count(b)>0){
				continue;
			}
			cout<<a.idToWord(m_wordSize,m_parameters->getColorSpaceMode())<<" -> "<<b.idToWord(m_wordSize,m_parameters->getColorSpaceMode())<<endl;
			printedEdges[a].insert(b);
		}
	}
	cout<<"}"<<endl;
}

/**
 *
 */
bool BubbleTool::isGenuineBubble(Kmer root,vector<vector<Kmer > >*trees,
map<Kmer ,int>*coverages,int repeatCoverage){
	#ifdef NO_BUBBLES
	return false;
	#endif

	if((*coverages)[root]>= repeatCoverage){
		return false;
	}

	int m_wordSize=m_parameters->getWordSize();
	#ifdef ASSERT
	for(int i=0;i<(int)trees->size();i++){
		for(int j=0;j<(int)trees->at(i).size();j+=2){
			Kmer a=trees->at(i).at(j+0);
			Kmer b=trees->at(i).at(j+1);
			string as=a.idToWord(m_wordSize,m_parameters->getColorSpaceMode());
			string bs=b.idToWord(m_wordSize,m_parameters->getColorSpaceMode());
			assert(as.substr(1,m_wordSize-1)==bs.substr(0,m_wordSize-1));
		}
	}
	#endif

	if(m_parameters->debugBubbles()){
		printStuff(root,trees,coverages);
	}

	if(trees->size()!=2){
		return false;// we don'T support that right now ! triploid stuff are awesome.
	}

	// given the word size
	// check that they join.
	//
	// substitution SNP is d=0
	// del is 1, 2, or 3

	map<Kmer ,int> coveringNumber;

	Kmer target;
	bool foundTarget=false;
	for(int j=0;j<(int)trees->size();j++){
		for(int i=0;i<(int)trees->at(j).size();i+=2){
			Kmer a=trees->at(j).at(i+1);
			#ifdef ASSERT
			if(coverages->count(a)==0){
				cout<<a.idToWord(m_parameters->getWordSize(),m_parameters->getColorSpaceMode())<<" has no coverage."<<endl;
			}
			assert(coverages->count(a)>0);
			#endif

			coveringNumber[a]++;
			if(!foundTarget && coveringNumber[a]==2){
				foundTarget=true;
				target=a;
				break;
			}
		}
	}

	if(!foundTarget){
		if(m_parameters->debugBubbles()){
			cout<<"Target not found."<<endl;
		}
		return false;
	}

	if((*coverages)[target]>= repeatCoverage){
		return false;
	}

	#ifdef ASSERT
	assert(coverages->count(root)>0);
	assert(coverages->count(target)>0);
	#endif
	#ifdef ASSERT
	int rootCoverage=(*coverages)[root];
	int targetCoverage=(*coverages)[target];
	assert(rootCoverage>0);
	assert(targetCoverage>0);
	#endif

	vector<map<Kmer ,Kmer > > parents;

	for(int j=0;j<(int)trees->size();j++){
		map<Kmer ,Kmer > aVector;
		parents.push_back(aVector);
		for(int i=0;i<(int)trees->at(j).size();i+=2){
			Kmer a=trees->at(j).at(i+0);
			Kmer b=trees->at(j).at(i+1);
			parents[j][b]=a;
		}
	}

	vector<vector<int> > observedValues;
	/*
 *
 *  BUBBLE is below
 *
 *    *  ----  * -------*  --------*
 *      \                          /
 *        ---- * --------* ------ *
 *
 */
	// accumulate observed values
	// and stop when encountering
	for(int j=0;j<(int)trees->size();j++){
		vector<int> aVector;
		observedValues.push_back(aVector);
		set<Kmer > visited;
		
		Kmer startingPoint=trees->at(j).at(0);
		Kmer current=target;

		while(current!=startingPoint){
			if(visited.count(current)>0){
				return false;
			}
			visited.insert(current);
			Kmer theParent=parents[j][current];
			int coverageValue=(*coverages)[theParent];

			observedValues[j].push_back(coverageValue);
			current=theParent;
		}
	}

	if(m_parameters->debugBubbles()){
		cout<<"O1="<<observedValues[0].size()<<" O2="<<observedValues[1].size()<<endl;
	}

	int sum1=0;
	for(int i=0;i<(int)observedValues[0].size();i++){
		sum1+=observedValues[0][i];
	}

	if(m_parameters->debugBubbles()){
		cout<<"O1Values= ";
		for(int i=0;i<(int)observedValues[0].size();i++){
			cout<<observedValues[0][i]<<" ";
		}
		cout<<endl;
	}

	int sum2=0;
	for(int i=0;i<(int)observedValues[1].size();i++){
		sum2+=observedValues[1][i];
	}
	
	if(m_parameters->debugBubbles()){
		cout<<"O2Values= ";
		for(int i=0;i<(int)observedValues[1].size();i++){
			cout<<observedValues[1][i]<<" ";
		}
		cout<<endl;
	}

	if((int)observedValues[0].size()<2*m_parameters->getWordSize()
	&& (int)observedValues[1].size()<2*m_parameters->getWordSize()){
		if(sum1>sum2){
			m_choice=trees->at(0).at(1);
		}else if(sum2>sum1){
			m_choice=trees->at(1).at(1);

		// this will not happen often
		}else if(sum1==sum2){
			// take the shortest, if any
			if(observedValues[0].size()<observedValues[1].size()){
				m_choice=trees->at(0).at(1);
			}else if(observedValues[1].size()<observedValues[0].size()){
				m_choice=trees->at(1).at(1);
			// same length and same sum, won't happen very often anyway
			}else{
				m_choice=trees->at(0).at(1);
			}
		}
		
		if(m_parameters->debugBubbles()){
			cout<<"This is a genuine bubble"<<endl;
			cout<<"root="<<root.idToWord(m_wordSize,m_parameters->getColorSpaceMode())<<" target="<<target.idToWord(m_wordSize,m_parameters->getColorSpaceMode())<<endl;
		}

		return true;
	}

	if(m_parameters->debugBubbles()){
		cout<<"False at last"<<endl;
	}

	return false;
}

Kmer BubbleTool::getTraversalStartingPoint(){
	return m_choice;
}

void BubbleTool::constructor(Parameters*parameters){
	m_parameters=parameters;
}

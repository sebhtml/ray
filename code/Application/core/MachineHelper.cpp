/*
 	Ray
    Copyright (C) 2012 Sébastien Boisvert

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

#include <core/MachineHelper.h>
#include <communication/mpi_tags.h>
#include <communication/Message.h>
#include <graph/CoverageDistribution.h>

#include <map>
#include <sstream>
using namespace std;

void MachineHelper::call_RAY_MASTER_MODE_LOAD_CONFIG(){

	if(m_argc==2 && m_argv[1][0]!='-'){
		ifstream f(m_argv[1]);
		if(!f){
			cout<<"Rank "<<m_parameters->getRank()<<" invalid input file."<<endl;
			m_parameters->showUsage();
			(*m_aborted)=true;
			f.close();
			m_switchMan->setMasterMode(RAY_MASTER_MODE_KILL_ALL_MPI_RANKS);
			return;
		}
	}

	if(m_parameters->getError()){
		(*m_aborted)=true;
		m_switchMan->setMasterMode(RAY_MASTER_MODE_KILL_ALL_MPI_RANKS);
		return;
	}

	uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(2*sizeof(uint64_t));
	message[0]=m_parameters->getWordSize();
	message[1]=m_parameters->getColorSpaceMode();

	for(int i=0;i<m_parameters->getSize();i++){
		Message aMessage(message,2,i,RAY_MPI_TAG_SET_WORD_SIZE,m_parameters->getRank());
		m_outbox->push_back(aMessage);
	}

	m_switchMan->setMasterMode(RAY_MASTER_MODE_TEST_NETWORK);
}

void MachineHelper::constructor(int argc,char**argv,Parameters*parameters,
SwitchMan*switchMan,RingAllocator*outboxAllocator,
		StaticVector*outbox,bool*aborted,
	map<int,uint64_t>*coverageDistribution,
	int*numberOfMachinesDoneSendingCoverage,
	int*numberOfRanksWithCoverageData
){
	m_argc=argc;
	m_argv=argv;
	m_parameters=parameters;

	m_switchMan=switchMan;
	m_outboxAllocator=outboxAllocator;
	m_outbox=outbox;

	m_aborted=aborted;

	m_coverageDistribution=coverageDistribution;
	m_numberOfMachinesDoneSendingCoverage=numberOfMachinesDoneSendingCoverage;

	m_numberOfRanksWithCoverageData=numberOfRanksWithCoverageData;
}

void MachineHelper::call_RAY_MASTER_MODE_SEND_COVERAGE_VALUES (){
	if(m_parameters->hasCheckpoint("GenomeGraph")){
		cout<<"Rank "<<m_parameters->getRank()<<" is reading checkpoint CoverageDistribution"<<endl;
		m_coverageDistribution->clear();
		ifstream f(m_parameters->getCheckpointFile("CoverageDistribution").c_str());
		int n=0;
		f.read((char*)&n,sizeof(int));
		int coverage=0;
		uint64_t count=0;
		for(int i=0;i<n;i++){
			f.read((char*)&coverage,sizeof(int));
			f.read((char*)&count,sizeof(uint64_t));
			(*m_coverageDistribution)[coverage]=count;
		}
		f.close();
	}

	if(m_parameters->writeCheckpoints() && !m_parameters->hasCheckpoint("CoverageDistribution")){
		cout<<"Rank "<<m_parameters->getRank()<<" is writing checkpoint CoverageDistribution"<<endl;
		ofstream f(m_parameters->getCheckpointFile("CoverageDistribution").c_str());
		int theSize=m_coverageDistribution->size();
		f.write((char*)&theSize,sizeof(int));
		for(map<int,uint64_t>::iterator i=m_coverageDistribution->begin();i!=m_coverageDistribution->end();i++){
			int coverage=i->first;
			uint64_t count=i->second;
			f.write((char*)&coverage,sizeof(int));
			f.write((char*)&count,sizeof(uint64_t));
		}
		f.close();
	}

	if(m_coverageDistribution->size()==0){
		cout<<endl;
		cout<<"Rank 0: Assembler panic: no k-mers found in reads."<<endl;
		cout<<"Rank 0: Perhaps reads are shorter than the k-mer length (change -k)."<<endl;
		(*m_aborted)=true;
		m_switchMan->setMasterMode(RAY_MASTER_MODE_KILL_ALL_MPI_RANKS);
		return;
	}
	(*m_numberOfMachinesDoneSendingCoverage)=-1;
	string file=m_parameters->getCoverageDistributionFile();
	CoverageDistribution distribution(m_coverageDistribution,&file);

	m_parameters->setMinimumCoverage(distribution.getMinimumCoverage());
	m_parameters->setPeakCoverage(distribution.getPeakCoverage());
	m_parameters->setRepeatCoverage(distribution.getRepeatCoverage());

	printf("\n");
	fflush(stdout);

	cout<<endl;
	cout<<"Rank "<<getRank()<<": the minimum coverage is "<<m_parameters->getMinimumCoverage()<<endl;
	cout<<"Rank "<<getRank()<<": the peak coverage is "<<m_parameters->getPeakCoverage()<<endl;

	uint64_t numberOfVertices=0;
	uint64_t verticesWith1Coverage=0;
	int lowestCoverage=9999;
	
	uint64_t genomeKmers=0;

	for(map<int,uint64_t>::iterator i=m_coverageDistribution->begin();
		i!=m_coverageDistribution->end();i++){
		int coverageValue=i->first;
		uint64_t vertices=i->second;
		if(coverageValue<lowestCoverage){
			verticesWith1Coverage=vertices;
			lowestCoverage=coverageValue;
		}
		if(coverageValue>=m_parameters->getMinimumCoverage()){
			genomeKmers+=vertices;
		}
		numberOfVertices+=vertices;
	}
	double percentageSeenOnce=(0.0+verticesWith1Coverage)/numberOfVertices*100.00;

	ostringstream g;
	g<<m_parameters->getPrefix();
	g<<"CoverageDistributionAnalysis.txt";
	ofstream outputFile(g.str().c_str());
	outputFile<<"k-mer length:\t"<<m_parameters->getWordSize()<<endl;
	outputFile<<"Lowest coverage observed:\t"<<lowestCoverage<<endl;
	outputFile<<"MinimumCoverage:\t"<<m_parameters->getMinimumCoverage()<<endl;
	outputFile<<"PeakCoverage:\t"<<m_parameters->getPeakCoverage()<<endl;
	outputFile<<"RepeatCoverage:\t"<<m_parameters->getRepeatCoverage()<<endl;
	outputFile<<"Number of k-mers with at least MinimumCoverage:\t"<<genomeKmers<<" k-mers"<<endl;
	outputFile<<"Estimated genome length:\t"<<genomeKmers/2<<" nucleotides"<<endl;
	outputFile<<"Percentage of vertices with coverage "<<lowestCoverage<<":\t"<<percentageSeenOnce<<" %"<<endl;
	outputFile<<"DistributionFile: "<<file<<endl;

	outputFile.close();

	m_coverageDistribution->clear();

	// display a warning
	// for RNA-Seq and for méta-genomes, this is not important
	//
	if(m_parameters->getMinimumCoverage()> m_parameters->getPeakCoverage()
	|| m_parameters->getPeakCoverage()==m_parameters->getRepeatCoverage()
	|| m_parameters->getPeakCoverage()==1){
		cout<<"Warning: no peak observed in the k-mer coverage distribution."<<endl;
		cout<<"to deal with the sequencing error rate, try to lower the k-mer length (-k)"<<endl;
		cout<<"If you are using RNA-Seq or metagenomic data, then you can ignore this warning."<<endl;
	}

	// see these values to everyone.
	uint64_t*buffer=(uint64_t*)m_outboxAllocator->allocate(3*sizeof(uint64_t));
	buffer[0]=m_parameters->getMinimumCoverage();
	buffer[1]=m_parameters->getPeakCoverage();
	buffer[2]=m_parameters->getRepeatCoverage();

	(*m_numberOfRanksWithCoverageData)=0;
	for(int i=0;i<m_parameters->getSize();i++){
		Message aMessage(buffer,3,i,RAY_MPI_TAG_SEND_COVERAGE_VALUES,getRank());
		m_outbox->push_back(aMessage);
	}
	m_switchMan->setMasterMode(RAY_MASTER_MODE_DO_NOTHING);
}

int MachineHelper::getRank(){
	return m_parameters->getRank();
}

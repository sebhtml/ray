/*
 	Ray
    Copyright (C) 2010, 2011, 2012 Sébastien Boisvert

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

#include <plugin_Searcher/DistributionWriter.h>
#include <application_core/constants.h>
#include <application_core/common_functions.h>

#include <sstream>
#include <iostream>
using namespace std;

#ifdef ASSERT
#include <assert.h>
#endif

DistributionWriter::DistributionWriter(){
	m_gotFile=false;
}

void DistributionWriter::setBase(const char*base){
	m_base=base;
}

void DistributionWriter::setRank(Rank rank){
	m_rank=rank;
}

void DistributionWriter::write(int directory,int file,int sequence,
	map<CoverageDepth,LargeCount>*all,map<CoverageDepth,LargeCount>*uniquelyColored,map<CoverageDepth,LargeCount>*uniquelyColoredAndAssembled,
	const char*directoryName,const char*fileName){

	openFile();

	m_output_Buffer<<"<entry><directory>";
	m_output_Buffer<<directoryName<<"</directory><file>"<<fileName<<"</file><sequence>";
	m_output_Buffer<<sequence<<"</sequence>"<<endl;

	m_output_Buffer<<"<raw>"<<endl;

	m_output_Buffer<<"#Coverage depth	Frequency"<<endl;

	for(map<CoverageDepth,LargeCount>::iterator i=all->begin();
		i!=all->end();i++){

		m_output_Buffer<<i->first<<"	"<<i->second<<endl;
	}
	
	m_output_Buffer<<"</raw>"<<endl;

	m_output_Buffer<<"<uniquelyColored>"<<endl;

	m_output_Buffer<<"#Coverage depth	Frequency"<<endl;

	for(map<CoverageDepth,LargeCount>::iterator i=uniquelyColored->begin();
		i!=uniquelyColored->end();i++){

		m_output_Buffer<<i->first<<"	"<<i->second<<endl;
	}

	m_output_Buffer<<"</uniquelyColored>"<<endl;

	m_output_Buffer<<"<uniquelyColoredAndAssembled>"<<endl;
	m_output_Buffer<<"#Coverage depth	Frequency"<<endl;

	for(map<CoverageDepth,LargeCount>::iterator i=uniquelyColoredAndAssembled->begin();
		i!=uniquelyColoredAndAssembled->end();i++){

		m_output_Buffer<<i->first<<"	"<<i->second<<endl;
	}

	m_output_Buffer<<"</uniquelyColoredAndAssembled>"<<endl;
	m_output_Buffer<<"</entry>"<<endl;

	flushFileOperationBuffer(false,&m_output_Buffer,&m_output,CONFIG_FILE_IO_BUFFER_SIZE);
}

void DistributionWriter::close(){
	if(m_gotFile){
		m_output_Buffer<<"</root>"<<endl;

		flushFileOperationBuffer(true,&m_output_Buffer,&m_output,CONFIG_FILE_IO_BUFFER_SIZE);

		m_output.close();

		m_gotFile=false;

		cout<<"[IO] Rank "<<m_rank<<" performed "<<m_operations<<" input/output operations on the file system from DistributionWriter."<<endl;

	}
}

void DistributionWriter::openFile(){
	if(m_gotFile){
		return;
	}

	m_operations=0;

	ostringstream rawDistribution;
	rawDistribution<<m_base<<"/"<<m_rank<<".Distributions.xml";
	m_output.open(rawDistribution.str().c_str());

	cout<<"Opened "<<rawDistribution.str()<<endl;

	m_output_Buffer<<"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"<<endl;
	m_output_Buffer<<"<root>"<<endl;

	m_gotFile=true;
}

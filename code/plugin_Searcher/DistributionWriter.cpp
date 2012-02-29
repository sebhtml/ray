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

#include <sstream>
#include <iostream>
using namespace std;

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
	map<int,uint64_t>*all,map<int,uint64_t>*uniquelyColored,map<int,uint64_t>*uniquelyColoredAndAssembled){

	openFile();

	m_output<<"<entry><directory>";
	m_output<<directory<<"</directory><file>"<<file<<"</file><sequence>";
	m_output<<sequence<<"</sequence>"<<endl;

	m_output<<"<raw>"<<endl;

	m_output<<"#Coverage depth	Frequency"<<endl;

	for(map<int,uint64_t>::iterator i=all->begin();
		i!=all->end();i++){

		m_output<<i->first<<"	"<<i->second<<endl;
	}
	
	m_output<<"</raw>"<<endl;

	m_output<<"<uniquelyColored>"<<endl;

	m_output<<"#Coverage depth	Frequency"<<endl;

	for(map<int,uint64_t>::iterator i=uniquelyColored->begin();
		i!=uniquelyColored->end();i++){

		m_output<<i->first<<"	"<<i->second<<endl;
	}

	m_output<<"</uniquelyColored>"<<endl;

	m_output<<"<uniquelyColoredAndAssembled>"<<endl;
	m_output<<"#Coverage depth	Frequency"<<endl;

	for(map<int,uint64_t>::iterator i=uniquelyColoredAndAssembled->begin();
		i!=uniquelyColoredAndAssembled->end();i++){

		m_output<<i->first<<"	"<<i->second<<endl;
	}

	m_output<<"</uniquelyColoredAndAssembled>"<<endl;
	m_output<<"</entry>"<<endl;
}

void DistributionWriter::close(){
	if(m_gotFile){
		m_output<<"</root>"<<endl;

		m_output.close();

		m_gotFile=false;
	}
}

void DistributionWriter::openFile(){
	if(m_gotFile)
		return;

	ostringstream rawDistribution;
	rawDistribution<<m_base<<"/"<<m_rank<<".Distributions.xml";
	m_output.open(rawDistribution.str().c_str());

	cout<<"Opened "<<rawDistribution.str()<<endl;

	m_output<<"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"<<endl;
	m_output<<"<root>"<<endl;

	m_gotFile=true;
}

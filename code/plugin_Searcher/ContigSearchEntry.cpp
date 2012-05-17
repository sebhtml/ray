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
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>
*/

#include <plugin_Searcher/ContigSearchEntry.h>


ContigSearchEntry::ContigSearchEntry(PathHandle name,int length,int mode,double mean,int coloredKmers){
	m_name=name;
	m_length=length;
	m_modeCoverage=mode;
	m_meanCoverage=mean;

	m_coloredKmers=coloredKmers;
}

PathHandle ContigSearchEntry::getName(){
	return m_name;
}

int ContigSearchEntry::getLength(){
	return m_length;
}

int ContigSearchEntry::getMode(){
	return m_modeCoverage;
}

double ContigSearchEntry::getMean(){
	return m_meanCoverage;
}

int ContigSearchEntry::getTotal(){
	// the repeated k-mers are considered once
	return getMode()*getLength();
}

void ContigSearchEntry::write(ofstream*file,LargeCount total,int kmerLength){
	double ratio=getTotal();

	// be careful with those 0s
	if(total!=0)
		ratio/=total;

	(*file)<<"contig-"<<getName()<<"	"<<kmerLength;
	(*file)<<"	"<<getLength();
	(*file)<<"	"<<m_coloredKmers;

	double ratioColored=m_coloredKmers;
	if(getLength()!=0){
		ratioColored/=getLength();
	}

	(*file)<<"	"<<ratioColored;
	(*file)<<"	"<<getMode();
	//(*file)<<"	"<<getMean();
	(*file)<<"	"<<getTotal()<<"	";
	(*file)<<total<<"	"<<ratio<<endl;
}

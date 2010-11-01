/*
 	Ray
    Copyright (C) 2010  Sébastien Boisvert

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


#include<TimePrinter.h>
#include<iostream>
using namespace std;

void TimePrinter::printElapsedTime(){
	time_t m_endingTime=time(NULL);
	int difference=m_endingTime-m_startingTime;
	int minutes=difference/60;
	int seconds=difference%60;
	int hours=minutes/60;
	minutes=minutes%60;
	int days=hours/24;
	hours=hours%24;
	bool started=false;
	cout<<"Rank 0: ELAPSED TIME: ";
	if(days>0){
		cout<<days<<" days";
		if(!started){
			started=true;
			cout<<", ";
		}
	}
	if(hours>0){
		cout<<hours<<" hours";
		if(!started){
			started=true;
			cout<<", ";
		}

	}
	if(minutes>0){
		cout<<minutes<<" minutes";
		if(!started){
			started=true;
			cout<<", ";
		}

	}
	if(seconds>0){
		cout<<seconds<<" seconds";
	}
	int differenceWithLast=m_endingTime-m_lastTime;
	cout<<" (+"<<differenceWithLast<<" seconds from last epoch)"<<endl;
	
	m_lastTime=m_endingTime;
}

TimePrinter::TimePrinter(){
	m_startingTime=m_lastTime=m_endingTime=time(NULL);
}

#include <tests/unitTest.h>
#include <core/common_functions.h>
#include <stdint.h>
#include <iostream>
using namespace std;

void test1(){
	// description of m_edges:
	// outgoing  ingoing
	//
	// G C T A G C T A
	//
	// 7 6 5 4 3 2 1 0

	uint8_t edges=1<<1;
	uint8_t expected=1<<4;
	uint8_t actual=invertEdges(edges);
	assertEquals(actual,expected);
	//cout<<"In="<<endl;
	//print(edges);
	//cout<<"Out="<<endl;
	//print(res);
}

void test2(){
	uint8_t edges=1<<2;
	uint8_t expected=1<<7;
	uint8_t actual=invertEdges(edges);
	assertEquals(actual,expected);

}

/*
 * In: 
 * G C T A G C T A
 * 7 6 5 4 3 2 1 0
 * 0 0 1 0 0 0 0 0 
 * Out: 
 * 0 0 0 0 0 1 0 0
 */

void test3(){
	uint8_t edges=1<<5;
	uint8_t expected=1<<0;
	uint8_t actual=invertEdges(edges);
	assertEquals(actual,expected);

}


int main(){
	test1();
	test2();
	test3();
	return 0;
}

#include<assert.h>
#include<SequencesLoader.h>
#include<stdlib.h>

int main(){
	int a=90;
	int b=133;
	int c=9;
	int d=1921;

	PaddedData p;
	p.medium[0]=a;
	p.medium[1]=b;
	p.medium[2]=c;
	p.medium[3]=d;

	PaddedData q;
	q.large[0]=p.large[0];
	q.large[1]=p.large[1];

	assert(q.medium[0]==a);
	assert(q.medium[1]==b);
	assert(q.medium[2]==c);
	assert(q.medium[3]==d);

	return EXIT_SUCCESS;
}

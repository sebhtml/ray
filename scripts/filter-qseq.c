/*

To compile this file under GNU/Linux with gcc:


	gcc filter-qseq.c  -Wall -ansi

I used this C program to filter Assemblathon 2/Parrot/Illumina UK/TruSeq 3 export qseq data files.

Author: Sébastien Boisvert
Distributed with the Ray assembler
License: GPL
2011-06-23 
^^
*/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int valid(char*a){
	int i=0;
	int l=strlen(a);
	while(i<l){
		char b=a[i];
		i++;
		if(b=='A'||b=='T'||b=='C'||b=='G'){
			continue;
		}
		return 0;
	}
	return 1;
}

/*
takes 2 Illumina qseq file and only keep pairs that don't have the '.' character
*/
int main(int argc,char**argv){
	if(argc!=3){
		printf("takes 2 Illumina qseq file and only keep pairs that don't have the '.' character\n");
		return 0;
	}
	char*file1=argv[1];
	char*file2=argv[2];
	char*sequence1=(char*)malloc(4096);
	char*sequence2=(char*)malloc(4096);
	char*trash=(char*)malloc(4096);

	FILE*fp1=fopen(file1,"r");
	FILE*fp2=fopen(file2,"r");

	char*fastq1=(char*)malloc(1000);
	char*fastq2=(char*)malloc(1000);

	char extension[]=".fastq";

	strcpy(fastq1,file1);
	strcpy(fastq2,file2);
	
	strcpy(fastq1+strlen(fastq1),extension);
	strcpy(fastq2+strlen(fastq2),extension);

	FILE*fout1=fopen(fastq1,"w");
	FILE*fout2=fopen(fastq2,"w");

	int qseqNumberOfColumns=11+1; /* change me */
	
	int qseqSequenceColumn=9;

	int pair=0;

	while(!feof(fp1)&&!feof(fp2)){
		int i=0;
		while(i++<qseqSequenceColumn)
			fscanf(fp1,"%s",sequence1);
		while(i++<qseqNumberOfColumns)
			fscanf(fp1,"%s",trash);
		
		i=0;
		while(i++<qseqSequenceColumn)
			fscanf(fp2,"%s",sequence2);
		while(i++<qseqNumberOfColumns)
			fscanf(fp2,"%s",trash);
		
		if(!valid(sequence1))
			continue;
		if(!valid(sequence2))
			continue;

		fprintf(fout1,">%i/1\n%s\n",pair,sequence1);
		fprintf(fout2,">%i/1\n%s\n",pair,sequence2);

		pair++;
	}

	fclose(fp2);
	fclose(fp1);
	free(sequence1);
	free(sequence2);
	free(fastq1);
	free(fastq2);
	free(fout1);
	free(fout2);
	return 0;
}

#!/usr/bin/Rscript

arguments=commandArgs(trailingOnly = TRUE)

prefix=arguments[1]

d1=read.table(paste(prefix,".RawDistribution.tsv",sep=""),header=TRUE)
d2=read.table(paste(prefix,".UniquelyColoredDistribution.tsv",sep=""),header=TRUE)
d3=read.table(paste(prefix,".UniquelyColoredAssembledDistribution.tsv",sep=""),header=TRUE)


# find the peak
maxI=1
i=1

while(i<= length(d1[[1]]) ){
	if(d1[[2]][i] > d1[[2]][maxI]){
		maxI=i
	}

	i=i+1
}

threshold=32
i=maxI

while(i<= length(d1[[1]]) && d1[[2]][i] >= threshold){
	i=i+1
}

if(i > length(d1[[1]])){
	i=length(d1[[1]])
}

maxX=d1[[1]][i]

# generate non-sparse arrays

d1Full=1:d1[[1]][length(d1[[1]])] * 0

i=1
while(i <= length(d1[[1]])){
	x=d1[[1]][i]
	y=d1[[2]][i]

	if(x<=length(d1Full))
		d1Full[x]=y

	i=i+1
}

d2Full=1:d2[[1]][length(d2[[1]])] * 0

i=1
while(i <= length(d2[[1]])){
	x=d2[[1]][i]
	y=d2[[2]][i]

	if(x<= length(d2Full))
		d2Full[x]=y

	i=i+1
}


d3Full=1:d3[[1]][length(d3[[1]])] * 0

i=1
while(i <= length(d3[[1]])){
	x=d3[[1]][i]
	y=d3[[2]][i]

	if(x<=length(d3Full))
		d3Full[x]=y

	i=i+1
}

png(paste(prefix,".png",sep=""))
plot(d1[[1]],d1[[2]],type='b',col='black',xlim=c(0,maxX),xlab='K-mer coverage depth',
ylab='Frequency',main=paste("Distributions of k-mer coverage depth\n"),
log='y')

lines(d2[[1]],d2[[2]],col='blue',type='b')
lines(d3[[1]],d3[[2]],col='green',type='b')

dev.off()



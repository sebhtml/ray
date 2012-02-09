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

png(paste(prefix,".png",sep=""))
plot(d1[[1]],d1[[2]],type='l',col='black',xlim=c(0,maxX),xlab='K-mer coverage depth',
ylab='Frequency',main='Distributions of k-mer coverage depth',log='y')
lines(d2[[1]],d2[[2]],col='blue')
lines(d3[[1]],d3[[2]],col='green')
dev.off()


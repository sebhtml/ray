k=15
file=Ecoli-k=$k.fasta-TheCoverageDistribution.tab
echo "

findMaxIndex=function(yValues){
	i=10
	max=0
	maxId=0
	while(i<=length(yValues)-1){
		y=yValues[i]
		if(y>max){
			max=y
			maxId=i
		}
		i=i+1
	}
	maxId
}

getLeap=function(k){
	if(k%%2==0){
		1
	}else{
		0.5
	}
}

r=read.table('$file')
maxIndex=findMaxIndex(r[[2]])
png('SRA001125-.png')
plot(r[[1]],log(r[[2]]),type='l',xlab='Coverage',ylab='log(number of k-mers',col=$k,main='k-mer size impacts coverage distribution')
leap=getLeap($k)
text(r[[1]][maxIndex],log(r[[2]][maxIndex])+leap,$k)
"
for k in $(seq 16 32)
do
	file=Ecoli-k=$k.fasta-TheCoverageDistribution.tab
	echo "r=read.table('$file')
maxIndex=findMaxIndex(r[[2]])
leap=getLeap($k)
lines(r[[1]],log(r[[2]]),col=$k)
text(r[[1]][maxIndex],log(r[[2]][maxIndex])+leap,$k)
"
done

echo "
dev.off()
"

	

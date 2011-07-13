#!/usr/bin/Rscript
#
# Input: PREFIX.LibraryX.txt
# Output:   PREFIX.LibraryX.txt.pdf


arguments=commandArgs(trailingOnly = TRUE)

file=arguments[1]

r=read.table(file)

derivatives=c()

i=2
derivatives[1]=0

# 1. find the places where dy/dx = 0
# 2. find the zero with maximum y
# 3. find the x left of the peak zero with min y

while(i<=length(r[[1]])){
	xi=r[[1]][i]
	yi=log(r[[2]][i])
	xi1=r[[1]][i-1]
	yi1=log(r[[2]][i-1])
	derivative=(yi-yi1)/(xi-xi1)
	derivatives[i]=derivative
	i=i+1
}

i=1
peakI=1
while(i<=length(r[[2]])){
	y=r[[2]][i]
	if(y>r[[2]][peakI]){
		peakI=i
	}
	i=i+1
}

print(peakI)

outputFile=paste(file,".pdf",sep="")
pdf(outputFile)
xMax=500
par(mfrow=c(2,1))
plot(r[[1]],r[[2]],type='l',col='black',xlab="Outer distance",ylab="Frequency",
	main=paste("Distribution of outer distances\n",file,sep=""))
grid(lty=1,lwd=2)

points(c(r[[1]][peakI]),c(r[[2]][peakI]),col="red")

plot(r[[1]],derivatives,type='l',col='green',ylim=c(-1,+1),xlab="Outer distance",ylab="Derivative")

grid(lty=1,lwd=2)

dev.off()

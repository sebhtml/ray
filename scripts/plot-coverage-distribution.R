#!/usr/bin/Rscript
#
# input: PREFIX.CoverageDistribution.txt
# output: PREFIX.CoverageDistribution.txt.pdf

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


minimums=c()
maximums=c()

i=2
while(i<=length(r[[1]])){
	derivative=derivatives[i]
	lastDerivative=derivatives[i-1]

	if(lastDerivative>0&&derivative<0){
		maximums=c(maximums,i)
	}
	if(lastDerivative<0&&derivative>0){
		minimums=c(minimums,i)
	}
	i=i+1
}

#print("Derivatives computed")
i=1
peakI=maximums[1]
while(i<=length(maximums)){
	index=maximums[i]
	y=r[[2]][index]
	if(y>r[[2]][peakI]){
		peakI=index
	}
	i=i+1
}

i=peakI
minI=peakI
while(i>=1){
	y=r[[2]][i]
	if(y<r[[2]][minI]){
		minI=i
	}
	i=i-1
}

i=1
maxI=peakI
while(i<=length(r[[1]])){
	if(i<peakI){
		i=i+1
		next
	}
	derivative=derivatives[i]

	if(r[[2]][i]<r[[2]][minI]/2){
		maxI=i
		break
	}

	if(derivative>0){
		maxI=i
		break
	}
	i=i+1
}

#print("Found peak")
#print(r[[1]][peakI])



#print(r[[1]][minI])
#print(zeros)

#print("Found minimum")

outputFile=paste(file,".png",sep="")
png(outputFile)
xMax=500
par(mfrow=c(2,1))
plot(r[[1]],log(r[[2]])/log(10),type='l',col='black',xlim=c(0,2*r[[1]][maxI]),xlab="Coverage value",ylab="Number of vertices (log scale, base 10)",
	main=paste("Distribution of coverage values\n",file,sep=""))
grid(lty=1,lwd=2)

points(c(r[[1]][peakI]),log(c(r[[2]][peakI])),col="red")
points(c(r[[1]][minI]),log(c(r[[2]][minI])),col="red")
points(c(r[[1]][maxI]),log(c(r[[2]][maxI])),col="red")

#print("Points showed")
#par(new=TRUE)

#print("Before second")
scale=1
plot(r[[1]],derivatives,type='l',col='green',xlim=c(0,2*r[[1]][maxI]),ylim=c(-scale,scale),xlab="Coverage value",ylab="Derivative")

#legend("topright",col=c("black","green"),lty=1,legend=c("Signal","Derivative"))
#print("Done legend")
grid(lty=1,lwd=2)

#scale=0.1
#plot(r[[1]],secondDerivatives,type='l',col='blue',ylim=c(-scale,scale),
#xlim=c(0,500),xlab="Coverage value",ylab="Second derivative")
#grid(lty=1,lwd=2)

#plot(r[[1]],integrals,type='l',col='red',xlim=c(0,500),xlab="Coverage value",ylim=c(0,integrals[length(r[[1]])-1]))
#grid(lty=1,lwd=2)
dev.off()

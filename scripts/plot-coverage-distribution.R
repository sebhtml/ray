#!/usr/bin/Rscript

arguments=commandArgs(trailingOnly = TRUE)

file=arguments[1]

r=read.table(file)

integral=0
integrals=c()
derivatives=c()

i=2
integrals[1]=0
derivatives[1]=0

# 1. find the places where dy/dx = 0
# 2. find the zero with maximum y
# 3. find the x left of the peak zero with min y

zeros=c()
lastDerivative=9999
while(i<=length(r[[1]])){
	xi=r[[1]][i]
	yi=log(r[[2]][i])
	xi1=r[[1]][i-1]
	yi1=log(r[[2]][i-1])
	area=(xi-xi1)*(yi+yi1)/2
	integral=integral+area
	derivative=(yi-yi1)/(xi-xi1)
	integrals[i]=integral
	derivatives[i]=derivative
	if(lastDerivative!=9999 &&
	((lastDerivative>0&&derivative<0)
	||(lastDerivative<0&&derivative>0))){
		#print("")
		#print(lastDerivative)
		#print(derivative)
		zeros=c(zeros,i)
	}
	lastDerivative=derivative
	i=i+1
}

i=1
peakI=zeros[1]
while(i<=length(zeros)){
	index=zeros[i]
	x=r[[1]][index]
	y=log(r[[2]][index])
	if(y>r[[2]][peakI]){
		peakI=index
	}
	i=i+1
}

#print(r[[1]][peakI])

i=peakI
minI=peakI
while(i>=1){
	y=log(r[[2]][i])
	if(y<r[[2]][minI]){
		minI=i
	}
	i=i-1
}

#print(r[[1]][minI])
#print(zeros)

outputFile=paste(file,".pdf",sep="")
pdf(outputFile)
xMax=500
plot(r[[1]],log(r[[2]]),type='l',col='black',xlim=c(0,500),xlab="Coverage value",ylab="Number of vertices",
	main=paste("Distribution of coverage values ",file,sep=""))

i=1
while(i<=length(zeros)){
	index=zeros[i]
	x=r[[1]][index]
	y=log(r[[2]][index])
	#points(c(x),c(y),col="red")
	i=i+1
}

points(c(r[[1]][peakI]),log(c(r[[2]][peakI])),col="red")

par(new=TRUE)

plot(r[[1]],derivatives,type='l',col='green',xlim=c(0,500),ylim=c(-1,+1),xaxt="n",yaxt="n",xlab="",ylab="")

legend("topright",col=c("black","green"),lty=1,legend=c("Signal","Derivative"))
grid(lty=1,lwd=2)


#plot(r[[1]],integrals,type='l',col='red',xlim=c(0,500),xlab="Coverage value",ylim=c(0,integrals[length(r[[1]])-1]))
#grid(lty=1,lwd=2)
#dev.off()

r=read.table('profile.txt')
pdf("Profile.pdf",width=10,height=6)
par(mfrow=c(3,1))
plot(r[[1]],r[[2]],type='l',main='Speed',col='blue')
plot(r[[1]],r[[3]],type='l',main='Sent',col='red')
plot(r[[1]],r[[4]],type='l',main='Received',col='green')
dev.off()


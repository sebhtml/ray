

theta=7
delta=25
epsilon=0.00
step=0.005

while epsilon<0.10:
	ds=(1-epsilon)**theta
	ds=ds*ds
	db=(1-epsilon)**delta
	print str(epsilon)+" "+str(db)+" "+str(ds)
	epsilon+=step

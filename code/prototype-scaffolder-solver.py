
vertices={}
children={}
parents={}

for i in open("mega-links.txt"):
	# [1,0]<stdout>:MEGA-LINK 5 F 2000006 F 212
	t=i.split()
	contig1=t[1]
	strand1=t[2]
	contig2=t[3]
	strand2=t[4]
	distance=t[5]
	vertices[contig1]=1
	vertices[contig2]=1
	edge=[contig1,strand1,contig2,strand2,distance]
	if contig1 not in children:
		children[contig1]=[]
	if contig2 not in parents:
		parents[contig2]=[]
	children[contig1].append(edge)
	parents[contig2].append(edge)
	otherStrand1='F'
	if strand1=='F':
		otherStrand1='R'
	otherStrand2='F'
	if strand2=='F':
		otherStrand2='R'
	otherEdge=[contig2,otherStrand2,contig1,otherStrand1,distance]
	if contig2 not in children:
		children[contig2]=[]
	if contig1 not in parents:
		parents[contig1]=[]
	children[contig2].append(otherEdge)
	parents[contig1].append(otherEdge)

colors={}
currentColor=0

colorDb={}

#initiate algorithm
for i in vertices.items():
	vertex=i[0]
	colors[vertex]=currentColor
	colorDb[currentColor]=[]
	colorDb[currentColor].append(vertex)
	currentColor+=1

# assign colors
for i in vertices.items():
	vertex=i[0]
	print "Assigning "+vertex
	if vertex not in children:
		continue
	print "Children:"
	for i in children[vertex]:
		print i
	if len(children[vertex])==1:
		child=children[vertex][0][2]
		print "One children"
		if len(parents[child])==1:
			print "One parent"
			currentColor=colors[vertex]
			childColor=colors[child]
			if currentColor==childColor:
					continue
			for i in colorDb[childColor]:
				print "Setting color of "+i+" to "+str(currentColor)
				colors[i]=currentColor
				colorDb[currentColor].append(i)
			colorDb[childColor]=[]

print "colors: "

colorsDone={}

# routine to print contigs from the colors
for i in vertices.items():
	vertex=i[0]
	color=colors[vertex]
	print str(color)+" "+vertex
	skip=False
	if vertex in parents:
		parent=0
		while parent<len(parents[vertex]):
			parentVertex=parents[vertex][parent][0]
			parentColor=colors[parentVertex]
			if parentColor==color:
				skip=True
			parent+=1
	if skip:
		continue
	#print "/Scaffold"
	#print " "+vertex+" F"
	currentStrand="F"
	currentVertex=vertex
	while currentVertex in children:
		if len(children[currentVertex])==1:
			childVertex=children[currentVertex][0][2]
			childColor=colors[childVertex]
			if childColor==color:
				#print " "+childVertex
				currentVertex=childVertex
			else:
				break
		else:
			break

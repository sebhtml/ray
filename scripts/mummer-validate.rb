#!/usr/bin/ruby

require 'set'
if ARGV.size!=3
	puts "usage "
	puts "script reference.fasta draft.fasta output.txt"
	exit
end

reference=ARGV[0]
draft=ARGV[1]
output=ARGV[2]


fileOutput=File.open output,"w+"
prefix="#{output}_prefix"

system "nucmer --prefix=#{prefix} #{reference} #{draft} "

system "show-coords -rcl #{prefix}.delta  > #{prefix}.coords"
file="#{prefix}.coords"

system "show-snps -Clr #{prefix}.delta > #{prefix}.snps"

theMismatches=0
indels=0
lineNumber=0
snpfile=File.new("#{prefix}.snps","r")
while(line=snpfile.gets)
	if lineNumber>=5
		tokens=line.split
		if tokens[1]=='.' || tokens[2]=='.'
			indels+=1
		else
			theMismatches+=1
		end
	end
	lineNumber+=1
end

snpfile.close

class MummerCoordLine
	def initialize line
		@line=line
	end
	def getQueryName
		@line.split(" ")[19-1].strip
	end
	def getQueryCoverage
		@line.split(" ")[16-1].strip.to_f/100.0
	end
	def getRawLine
		@line
	end
	def getRefName
		@line.split(" ")[18-1].strip
	end
	def getRefStart
		@line.split(" ")[1-1].strip.to_i
	end
	def getRefEnd
		@line.split(" ")[2-1].strip.to_i
	end
	def getRefLength
		@line.split(" ")[12-1].strip.to_i
	end
end

f=File.open file
5.times do
	f.gets
end

queries={}

mums=[]

while l=f.gets
	#puts l
	aLine=MummerCoordLine.new l
	if queries[aLine.getQueryName].nil?
		queries[aLine.getQueryName]=[]
	end
	queries[aLine.getQueryName]<< aLine
end
f.close

misassembled=0

queries.each do |name,hits|
	ok=false
	filteredHits=[]
	hits.each do |hit|
		#puts hit.getQueryCoverage.to_s
		if hit.getQueryCoverage>=0.982
			ok=true
		end
		if hit.getQueryCoverage>=0.01
			filteredHits<< hit
		end
		mums<< hit
	end
	if ok==false and filteredHits.size==2 and filteredHits[0].getQueryCoverage+filteredHits[1].getQueryCoverage >= 0.99
		# check if it is circular
		puts "is circular?"
		if filteredHits[0].getRefStart==1 and filteredHits[1].getRefEnd==filteredHits[1].getRefLength
			puts "It is circular"
			ok=true
		end
	end
	if ok==false
		fileOutput.puts name
		filteredHits.each do |hit|
			fileOutput.puts hit.getRawLine
		end
		if filteredHits.size>1
			misassembled+=1
		end
	end
end

fileOutput.puts "Total contigs: #{queries.size}"
fileOutput.puts "Misassembled contigs: #{misassembled}"

puts "#{mums.size} mums"

queryByReferenceSequence={}
mums.each do |mum|
	#puts mum
	if queryByReferenceSequence[mum.getRefName].nil?
		queryByReferenceSequence[mum.getRefName]=[]
	end
	queryByReferenceSequence[mum.getRefName]<< mum
end

puts "#{queryByReferenceSequence.size} references"

totalBases=0
totalBasesNotCovered=0
queryByReferenceSequence.each do |refName,mums|
	refBases=mums[0].getRefLength
	totalBases+=refBases
	positions=Set.new
	mums.each do |mum|
		i=mum.getRefStart
		j=mum.getRefEnd
		while i<=j
			positions.add i
			i+=1
		end
	end
	missingBases=refBases-positions.size

totalBasesNotCovered+=missingBases
	fileOutput.puts "#{refName} #{refBases} #{missingBases} #{1.0-missingBases.to_f/refBases}"
end

#system "rm #{prefix}.coords"
#system "rm #{prefix}.snps"
#system "rm #{prefix}.delta"

fileOutput.puts "totalBases=#{totalBases}"
fileOutput.puts "totalBasesNotCovered=#{totalBasesNotCovered}"
fileOutput.puts "All #{totalBases} #{totalBasesNotCovered} "
fileOutput.puts "Coverage=#{1.0-totalBasesNotCovered.to_f/totalBases}"
fileOutput.puts "totalGaps=#{indels}"
fileOutput.puts "totalMismatches=#{theMismatches}"
fileOutput.close

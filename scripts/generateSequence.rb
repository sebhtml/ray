#!/usr/bin/ruby

i=0
n=ARGV[0].to_i
s=""
while i<n
	j=rand(4)
	if j==0
		s<< 'A'
	elsif j==1
		s<< 'T'
	elsif j==2
		s<< 'C'
	elsif j==3
		s<< 'G'
	end
	i+=1
end

puts ">s0"
puts s

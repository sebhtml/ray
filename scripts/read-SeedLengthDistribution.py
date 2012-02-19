#!/usr/bin/python
"""Extract seed length distribution from file

Usage:
    read-SeadLengthDistribution input_file 
"""

# Importing modules
import sys

# Main
if __name__ == '__main__':
    try:
        input_file = sys.argv[1]
    except:
        print __doc__
        sys.exit(1)

    total = 0
    n = 0
    
    for line in open(input_file):
	    tokens = line.strip().split()
	    count = int(tokens[1])
	    total += int(tokens[0]) * count
	    n += count
	
    print total
    print n


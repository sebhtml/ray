#!/usr/bin/python
# encoding:utf8
# author: Sébastien Boisvert
# part of Ray distribution
"""This script takes two fastq files and interleaves them

Usage:
    interleave-fasta.py fasta_file1 fasta_file2
"""

# Importing modules
import sys

# Main
if __name__ == '__main__':
    try:
        file1 = sys.argv[1]
        file2 = sys.argv[2]
    except:
        print __doc__
        sys.exit(1)
    
    with open(file1) as f1:
        with open(file2) as f2:
            while True:
                line = f1.readline()
                if line.strip() == "":
                    break
                print line.strip()
                
                for i in xrange(3):
                    print f1.readline().strip()
                
                for i in xrange(4):
                    print f2.readline().strip()


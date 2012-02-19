#!/usr/bin/python
"""Dump the content of the 'selfMap.psl' file

Usage:
    dumpPsl.py
"""
# 2022    0       0       0       0       0       0       0       +       contig-0        2022    0       2022    contig-0        2022    0       2022    1       2022,   0,      0,


# Importing modules
import sys

# Main
if __name__ == '__main__':
    input_file="selfMap.psl"
    with open(input_file) as f:
        for line in f.readlines()[6:]:
            tokens = line.split()
            matches = int(tokens[0])
            query = tokens[9]
            target = tokens[13]
            queryLength = int(tokens[10])
            targetLength = int(tokens[14])
            
            if query == target:
                continue
            
            ratio = matches / float(queryLength)
            
            if ratio < 0.5 or queryLength < 5000:
                continue
            
            print line.strip()


#!/usr/bin/python
"""Get a sequence from a fasta file by specifying the sequence name

Usage:
    getSeq.py fasta_file sequence_name
"""

# Importing modules
import sys

# Main
if __name__ = '__main__':
    try:
        fasta_file = sys.argv[1]
        sequence_name = sys.argv[2]
    except:
        print __doc__
        sys.exit(1)

    dump = False

    for line in open(fasta_file):
        if line.startswith('>') and sequence_name in line:
        # The previous condition is dangerous in the case of a sequence name
        # that can be found as a substring of another name. See other option below:
        
        # if line.startswith('>') and line.strip().replace('>', '') == sequence_name:
            dump = True
        
        if dump:
            print line.strip()
            break

#!/usr/bin/python
"""Doc

Usage:
    removeBreaks.py fastaFile.fa output.fa
"""

# Importing modules
import sys

# Defining functions
def writeToFile(head, body, output, threshold, p):
    if len(body) >= threshold:
        output.write(head + "\n")
        i = 0
        columns = 80
        while i < len(body):
            output.write(body[i:(i + columns)])
            i += columns
    output.write("\n")

# Main
if __name__ == '__main__':
    try:
        input_file = sys.argv[1]
        output_file = sys.argv[2]
    except:
        print __doc__
        sys.exit(1)
    
    with open(input_file) as in_file:
        with open(output_file, "w+") as output:
            head = ""
            body = ""
            threshold = 0
            p = 1
            
            for line in in_file:
                if line.startswith('>'):
                    if head != "":
                        writeToFile(head,body,output,threshold,p)
                        p += 1
                    head = line.strip()
                    body = ""
                else:
                    body += line.strip()
            
            writeToFile(head, body, output, threshold, p)


source 0sim2-parameters.sh
source 0parameters.sh

Assemble.pl $reads $wordSize &> log
print-latex.sh $ref  lol EULER-SR

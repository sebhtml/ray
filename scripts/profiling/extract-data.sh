expression="\\[1,$2\\]"
cat $1|grep $expression|grep RAY|awk '{print $5" "$8" "$10" "$12" "$3}' 

grep Warning log.log|awk '{print $5" "$3}' > raw-data

cat raw-data |sort -r -n > mode.profile

echo "Distribution: "

awk '{print $2}' mode.profile|sort|uniq -c

echo "Raw data= mode.profile"

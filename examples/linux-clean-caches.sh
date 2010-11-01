sync
cat /proc/sys/vm/drop_caches > defaultValue
echo 3 > /proc/sys/vm/drop_caches
cat defaultValue > /proc/sys/vm/drop_caches

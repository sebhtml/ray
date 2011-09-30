grep "from previous" log.log |awk '{print $5" "$10" "$11" "$12" "$15}' > stats


cat stats|sort -n -r > sorted

less sorted

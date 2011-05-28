i=$1
suffix=$2

(
echo ""
echo "BEGIN $i $(date)"
echo ""
rm -f ./*.fasta ./*.afg ./*.fa

bash $i |& tee $i.log

echo ""
echo "THE_END $i $(date)"
echo ""
)|& tee $i-$suffix


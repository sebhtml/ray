i=$1
suffix=$2

(
echo ""
echo "BEGIN $i $(date)"
echo ""

rm -f ./*.fasta ./*.afg ./*.fa

bash $i |& tee $i.log

for j in $(ls *.fasta)
do
	mv $j $j.old
done

for j in $(ls *.afg)
do
	mv $j $j.old
done

echo ""
echo "THE_END $i $(date)"
echo ""
)|& tee $i-$suffix


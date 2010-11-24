i=$1
suffix=$2

(
echo "RegressionTest"
echo ""
echo "FILE = $i"
date
echo ""
echo "OH HAI"
echo ""
echo "======== BEGIN >>>"
echo ""
bash $i
echo ""
echo "======== END^^ >>>"
echo ""
echo "K THX BYE"
echo ""
echo "..."
date
echo ""
)|tee $i-$suffix


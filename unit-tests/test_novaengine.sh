g++ test_novaengine.cpp ../code/heuristics/RayNovaEngine.cpp -O3 -I ../code -o test_nova -I ..
for i in $(ls NovaEngine)
do
	./test_nova NovaEngine/$i
done

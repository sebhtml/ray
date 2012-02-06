mpicxx  test_novaengine.cpp ../code/plugin_SeedExtender/NovaEngine.cpp -O3 -I ../code -o test_nova -I .. \
-I ../RayPlatform ../RayPlatform/*/*.cpp

for i in $(ls NovaEngine)
do
	./test_nova NovaEngine/$i
done

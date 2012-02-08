mpicxx  peakFinder.cpp ../code/plugin_Library/LibraryPeakFinder.cpp -I ../code/ -o peaks -I ../RayPlatform ../RayPlatform/core/statistics.cpp

mpicxx  -g test_peakFinder.cpp ../code/plugin_Library/LibraryPeakFinder.cpp -I ../code/ -o test_peaks -I .. -I ../RayPlatform ../RayPlatform/core/statistics.cpp

for i in $(ls Libraries/*.test)
do
	echo "*********************************"
	echo "Testing $i"
	./test_peaks $(echo $i|sed 's/.test/.txt/g') $i
done


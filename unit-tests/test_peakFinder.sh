mpicxx  peakFinder.cpp ../code/plugin_Library/LibraryPeakFinder.cpp -I ../code/ -o peaks

mpicxx  test_peakFinder.cpp ../code/plugin_Library/LibraryPeakFinder.cpp -I ../code/ -o test_peaks -I ..

for i in $(ls Libraries/*.test)
do
	echo "Testing $i"
	./test_peaks $(echo $i|sed 's/.test/.txt/g') $i
done


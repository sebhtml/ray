
mpicxx  -g test_coloredPeaks.cpp ../code/plugin_Searcher/ColoredPeakFinder.cpp -I ../code/ -o test_coloredPeaks -I .. -I ../RayPlatform ../RayPlatform/core/statistics.cpp

prog=$(pwd)/test_coloredPeaks

cd data-for-unit-tests/colored-peaks

$prog TestSuite.txt

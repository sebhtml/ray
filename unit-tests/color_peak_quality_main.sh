
g++ color_peak_quality_main.cpp ../code/plugin_Searcher/QualityCaller.cpp -I . -I .. -I ../code  -o QualityCaller \
../RayPlatform/core/statistics.cpp -I ../RayPlatform -DASSERT \
 -DCONFIG_CALLER_VERBOSE \
 -DCONFIG_CALLER_VERBOSE_POINTS

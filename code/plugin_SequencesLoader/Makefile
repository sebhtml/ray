
SequencesLoader-y += plugin_SequencesLoader/SequencesLoader.o 
SequencesLoader-y += plugin_SequencesLoader/Read.o  plugin_SequencesLoader/ArrayOfReads.o 
SequencesLoader-y += plugin_SequencesLoader/ColorSpaceDecoder.o plugin_SequencesLoader/ColorSpaceLoader.o 
SequencesLoader-y += plugin_SequencesLoader/FastaLoader.o plugin_SequencesLoader/FastqLoader.o plugin_SequencesLoader/SffLoader.o  plugin_SequencesLoader/Loader.o 
SequencesLoader-$(HAVE_LIBBZ2) += plugin_SequencesLoader/BzReader.o  
SequencesLoader-$(HAVE_LIBBZ2) += plugin_SequencesLoader/FastqBz2Loader.o 
SequencesLoader-$(HAVE_LIBZ) += plugin_SequencesLoader/FastqGzLoader.o 

obj-y += $(SequencesLoader-y)





SequencesLoader-y += assembler/SequencesLoader_adapters.o 
SequencesLoader-y += assembler/SequencesLoader.o 
SequencesLoader-y += structures/Read.o  structures/ArrayOfReads.o 
SequencesLoader-y += format/ColorSpaceDecoder.o format/ColorSpaceLoader.o 
SequencesLoader-y += format/FastaLoader.o format/FastqLoader.o format/SffLoader.o  assembler/Loader.o 
SequencesLoader-$(HAVE_LIBBZ2) += compression/BzReader.o  
SequencesLoader-$(HAVE_LIBBZ2) += format/FastqBz2Loader.o 
SequencesLoader-$(HAVE_LIBZ) += format/FastqGzLoader.o 

obj-y += $(SequencesLoader-y)




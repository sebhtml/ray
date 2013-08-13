SequencesLoader-y += code/SequencesLoader/SequencesLoader.o
SequencesLoader-y += code/SequencesLoader/Read.o
SequencesLoader-y += code/SequencesLoader/ArrayOfReads.o
SequencesLoader-y += code/SequencesLoader/ColorSpaceDecoder.o
SequencesLoader-y += code/SequencesLoader/ColorSpaceLoader.o
SequencesLoader-y += code/SequencesLoader/FastaLoader.o
SequencesLoader-y += code/SequencesLoader/FastaLoaderForReads.o
SequencesLoader-y += code/SequencesLoader/FastqLoader.o
SequencesLoader-y += code/SequencesLoader/ExportLoader.o
SequencesLoader-y += code/SequencesLoader/LoaderInterface.o
SequencesLoader-y += code/SequencesLoader/LoaderFactory.o
SequencesLoader-y += code/SequencesLoader/SffLoader.o
SequencesLoader-y += code/SequencesLoader/Loader.o
SequencesLoader-y += code/SequencesLoader/BufferedReader.o
SequencesLoader-y += code/SequencesLoader/ReadHandle.o

SequencesLoader-$(CONFIG_HAVE_LIBBZ2) += code/SequencesLoader/BzReader.o
SequencesLoader-$(CONFIG_HAVE_LIBBZ2) += code/SequencesLoader/FastqBz2Loader.o
SequencesLoader-$(CONFIG_HAVE_LIBBZ2) += code/SequencesLoader/FastaBz2Loader.o
SequencesLoader-$(CONFIG_HAVE_LIBZ) += code/SequencesLoader/FastqGzLoader.o
SequencesLoader-$(CONFIG_HAVE_LIBZ) += code/SequencesLoader/FastaGzLoader.o

SequencesLoader-y += code/SequencesLoader/SequenceFileDetector.o

obj-y += $(SequencesLoader-y)

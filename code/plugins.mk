plugins: ray_core.o $(PLUGINS-y)

ray_core.o: $(core-y)
	$(LD) -r $^ -o $@

ray_plugin_VerticesExtractor.o: $(VerticesExtractor-y)
	$(LD) -r $^ -o $@

ray_plugin_EdgePurger.o: $(EdgePurger-y)
	$(LD) -r $^ -o $@

ray_plugin_KmerAcademyBuilder.o: $(KmerAcademyBuilder-y)
	$(LD) -r $^ -o $@

ray_plugin_SequencesLoader.o: $(SequencesLoader-y)
	$(LD) -r $^ -o $@

ray_plugin_FusionTaskCreator.o: $(FusionTaskCreator-y)
	$(LD) -r $^ -o $@

ray_plugin_FusionData.o: $(FusionData-y)
	$(LD) -r $^ -o $@

ray_plugin_Scaffolder.o: $(Scaffolder-y)
	$(LD) -r $^ -o $@

ray_plugin_Searcher.o: $(Searcher-y)
	$(LD) -r $^ -o $@


ray_plugin_CoverageGatherer.o: $(CoverageGatherer-y)
	$(LD) -r $^ -o $@

ray_plugin_Partitioner.o: $(Partitioner-y)
	$(LD) -r $^ -o $@

ray_plugin_MessageProcessor.o: $(MessageProcessor-y)
	$(LD) -r $^ -o $@

ray_plugin_SequencesIndexer.o: $(SequencesIndexer-y)
	$(LD) -r $^ -o $@

ray_plugin_Amos.o: $(Amos-y)
	$(LD) -r $^ -o $@


ray_plugin_NetworkTest.o: $(NetworkTest-y)
	$(LD) -r $^ -o $@

ray_plugin_JoinerTaskCreator.o: $(JoinerTaskCreator-y)
	$(LD) -r $^ -o $@

ray_plugin_MachineHelper.o: $(MachineHelper-y)
	$(LD) -r $^ -o $@

ray_plugin_SeedingData.o: $(SeedingData-y)
	$(LD) -r $^ -o $@

ray_plugin_Library.o: $(Library-y)
	$(LD) -r $^ -o $@

ray_plugin_SeedExtender.o: $(SeedExtender-y)
	$(LD) -r $^ -o $@


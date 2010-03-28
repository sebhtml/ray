#!/bin/bash

currentPlace=$(pwd)
revision=3159

function runTest {
	startDate=$(date)
	echo ""
	echo "***************"
	nproc=30
	date
	testName=$1
	dataDirectory=$2
	rayTemplate=$3
	reference=$4
	description=$5
	echo "Running" $testName
	echo "Description: $description"
	echo "Directory=$dataDirectory"
	
	cd $dataDirectory
	echo "Input (from $rayTemplate):"
	cat $rayTemplate
	touch Log
	mpirun -tag-output  -np $nproc Ray$revision $rayTemplate > Log
	mv Log $currentPlace/$testName.Log
	print-latex.sh $reference Ray-Contigs.fasta  Ray > Ray.LatexLine
	mv Ray.LatexLine $currentPlace/$testName.Validation
	mv mums $currentPlace/$testName.Mums
	echo "Validating with $reference"
	mv Ray-CoverageDistribution.txt $currentPlace/$testName.CoverageDistribution
	mv Ray-Parameters.txt $currentPlace/$testName.Parameters
	mv Ray-Contigs.fasta $currentPlace/$testName.Contigs
	(
	echo "TestName: $testName"
	echo "Description: $description"
	echo "TestSet: $currentPlace"
	echo "Nproc: $nproc"
	echo "Host: $(hostname)"
	echo "StartingDate: $startDate"
	echo "EndingData: $(date)"
	echo "WorkingDirectory: $dataDirectory"
	echo "RayInputFile: $rayTemplate"
	echo "RayRevision: $revision"
	echo "ReferenceFile: $reference"
	)> $currentPlace/$testName.Descriptors
}

runTest Sp /data/users/boiseb01/PaperDatasets/Sp input /home/boiseb01/nuccore/Streptococcus-pneumoniae-R6.fasta "S. pneumoniae 50-nt reads @ 50X (nuccore/NC_003098)"
runTest SpEr /data/users/boiseb01/PaperDatasets/SpEr input /home/boiseb01/nuccore/Streptococcus-pneumoniae-R6.fasta "S. pneumoniae 50-nt reads @ 50X, 1% random mismatches (nuccore/NC_003098)"
runTest SpP /data/users/boiseb01/PaperDatasets/SpPaired input /home/boiseb01/nuccore/Streptococcus-pneumoniae-R6.fasta " S. pneumoniae paired 200 50-nt reads @ 50X (nuccore/NC_003098)"
runTest E400 /data/users/boiseb01/PaperDatasets/Ecoli400 input /home/boiseb01/nuccore/Ecoli-k12-mg1655.fasta " E. coli K-12 MG1655 400-nt reads @ 50X (nuccore/NC_000913) "
runTest MG1655-200-50 /data/users/boiseb01/PaperDatasets/MG1655-200-50 input /home/boiseb01/nuccore/Ecoli-k12-mg1655.fasta "#  E. coli K-12 MG1655 50-nt paired 200 reads @ 50X (nuccore/NC_000913) "
runTest MG1655-50 /data/users/boiseb01/PaperDatasets/MG1655-200-50 input-np /home/boiseb01/nuccore/Ecoli-k12-mg1655.fasta "#  E. coli K-12 MG1655 50-nt reads @ 50X (nuccore/NC_000913) "
runTest PAO1-p /data/users/boiseb01/PaperDatasets/Pseudo input /home/boiseb01/nuccore/Pseudomonas-aeruginosa-PAO1,-complete-genome.fasta "#  Pseudo 40-nt paired (250) reads @ 50 X (nuccore/NC_002516) "
runTest PAO1-np /data/users/boiseb01/PaperDatasets/Pseudo input-np /home/boiseb01/nuccore/Pseudomonas-aeruginosa-PAO1,-complete-genome.fasta "#  Pseudo 40-nt reads @ 50 X (nuccore/NC_002516) "




runTest Ep-Illumina /data/users/boiseb01/PaperDatasets/SRA001125 input /home/boiseb01/nuccore/Ecoli-k12-mg1655.fasta "Mixed set 1: E. coli K-12 MG1655 #  Illumina paired "
runTest Enp-Illumina /data/users/boiseb01/PaperDatasets/SRA001125 input-notpaired /home/boiseb01/nuccore/Ecoli-k12-mg1655.fasta "Mixed set 1: E. coli K-12 MG1655  Illumina non-paired "
runTest MG1655-454 /data/users/sra/SRX000348-MG1655-454 input /home/boiseb01/nuccore/Ecoli-k12-mg1655.fasta "#  Mixed set 1: E. coli K-12 MG1655 454"
runTest MG1655-mix /data/users/sra/SRX000348-MG1655-454 input-mix /home/boiseb01/nuccore/Ecoli-k12-mg1655.fasta "Mixed set 1: E. coli K-12 MG1655 #  454+ Illumina paired "

runTest adp1-roche /data/users/boiseb01/PaperDatasets/SRA003611 roche.Ray /home/boiseb01/nuccore/adp1.fasta "Mixed set 2: Acinetobacter sp. ADP1 454"
runTest adp1-Illumina /data/users/boiseb01/PaperDatasets/SRA003611 Illumina.ray /home/boiseb01/nuccore/adp1.fasta "Mixed set 2: Acinetobacter sp. ADP1  #  Illumina non-paired "
runTest adp1-mix /data/users/boiseb01/PaperDatasets/SRA003611 template.ray /home/boiseb01/nuccore/adp1.fasta "Mixed set 2: Acinetobacter sp. ADP1 #  454 + Illumina "

runTest crypto-illumina /data/users/sra/MarkChaisson/ Illumina /home/boiseb01/nuccore/Cryptobacterium_curtum_DSM_15641.fasta "Mixed set 3: Cryptobacterium curtum DSM 15641 Illumina"
runTest crypto-454 /data/users/sra/MarkChaisson/ roche454 /home/boiseb01/nuccore/Cryptobacterium_curtum_DSM_15641.fasta "Mixed set 3: Cryptobacterium curtum DSM 15641 454"
runTest crypto-mix /data/users/sra/MarkChaisson/ mix /home/boiseb01/nuccore/Cryptobacterium_curtum_DSM_15641.fasta "Mixed set 3: Cryptobacterium curtum DSM 15641 mix"


runTest myco-Illumina /data/users/boiseb01/PaperDatasets/SRA003611 Mycoplasma.Ray /home/boiseb01/nuccore/Mycoplasma_agalactiae_PG2.fasta "   1.  mycoplasma (genome: nuccore/NC_009497.1, reads: sra/SRA003611) "

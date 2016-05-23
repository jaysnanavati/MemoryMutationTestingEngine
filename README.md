<h2>This is a Mutation Testing engine for C language<h2>

<b>Published:</b> http://www0.cs.ucl.ac.uk/staff/mharman/mutation2015.pdf, http://www.sciencedirect.com/science/article/pii/S0950584916300362

<h2>Setup<h2>

To run this engine, you will need to download and install TXL: http://www.txl.ca/

This tool requires the Programs Under Test using CuTest as their unit-testing library. (See examples in PUTs/)

This repository comes with 16 example subjects, located in PUTs/

Instructions to run this tool step by step:

1. Go to the subject directory and create or modify a file "config.cfg" (e.g. PUTs/01pwp/config.cfg).
2. Write down configuration information in "config.cfg" (01pwp as example):
	sourceRootDir="/absolute/path/to/PUTs/01pwp";
	executablePath="01pwp/tests_connection";
	source = ("pwp_connection.c","pwp_handshaker.c","pwp_msghandler.c");
	testingFramework="cutest";
	CuTestLibSource="tests/CuTest.c";
3. After configuring all subjects, go to PUTs/ directory and run genSubjectList.sh. This will generate a file "subjectList.txt" containing a list of subjects under PUTs/. You can delete subjects that you don't want to mutant from this file. Then copy this file to Mutate/
4. Compile the tool:
	make
5. Run the tool with specified Mutation Operators:
	./run.sh ABS
   This will start Mutation Testing on all the subjects in "subjectList.txt" using Mutation Operator ABS. Currently we provide the following Mutation Operators:
   	ABS, AOR, LCR, UOI, ROR, M
   The last one is a set of Memory Mutation Operators (default). We don't support multiple Mutation Operators at one run yet.
6. When finished, there will be a summary file "gstats_ABS.xml" and a directory of raw data "mutation_out_ABS" in each subject directory (e.g. PUTs/01pwp/). "gstats_ABS.xml" contains the number of generated mutants, number of strongly killed mutants, number of weakly killed mutants etc. for the specified Mutation Operator (if the postfix is "M", gstats.xml has the same information for each of Memory Mutation Operators).

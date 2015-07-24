echo Performing mutant filtering...

cFile=avl_tree

for mutant in ABS_1115 ABS_31 ABS_146 ABS_33 ABS_172 ABS_32 ABS_189 UOI_346 UOI_136 UOI_138 UOI_345 UOI_340 UOI_140 UOI_270 UOI_1133 UOI_1127 UOI_1136 UOI_072 UOI_075 UOI_079 UOI_179 UOI_3122 UOI_336 UOI_277 UOI_272 UOI_135 UOI_145 UOI_275
do
	if [ -f mutation_out/$cFile/$mutant.c ]; then
		rm mutation_out/$cFile/$mutant.c
		echo remove $mutant.c
	fi
done

make_single_dir=../../PUTs0/$1
cwd=`pwd`

for opDir in `ls`; do
	if [[ $opDir = mutation_out_* ]]; then
		for srcDir in `ls $opDir`; do
			if [[ $srcDir = PUT ]]; then
				continue
			fi
			for mutant in `ls $opDir/$srcDir`; do
				if [[ $mutant = *.c ]]; then
					cp $opDir/$srcDir/$mutant $make_single_dir/$srcDir.c
					cd $make_single_dir
					rm -f mutant.o
					./make_single.sh $srcDir.c mutant.o
					if [ -f mutant.o ]; then
						cp $make_single_dir/mutant.o $cwd/$opDir/$srcDir/$mutant.o
					else
						echo $1,$opDir,$srcDir,$mutant,-1
					fi
					cd $cwd
			
				fi
			done
		done
	fi
done

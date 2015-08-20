cwd=`pwd`

for opDir in `ls`; do
	if [[ $opDir = mutation_out_* ]]; then
		for srcDir in `ls $opDir`; do
			if [[ $srcDir = PUT ]]; then
				continue
			fi
			echo $opDir/$srcDir 1>&2
			for mutant in `ls $opDir/$srcDir`; do
				if [[ $mutant = *.o ]]; then
					found=0
					for opDir2 in `ls`; do
						if [[ $opDir2 = mutation_out_* ]]; then
							for mutant2 in `ls $opDir2/$srcDir`; do
								if [[ $mutant2 = *.o ]]; then
									if [[ $mutant = $mutant2 ]]; then
										continue
									fi
									diff $opDir/$srcDir/$mutant $opDir2/$srcDir/$mutant2 > /dev/null
									if [ $? -eq 0 ]; then
										found=1
										echo $1,$opDir,$srcDir,$mutant,$mutant2
									fi 
								fi
							done
						fi
					done
					if [ $found -eq 0 ]; then
						echo $1,$opDir,$srcDir,$mutant,no match
					fi
				fi
			done
		done
	fi
done

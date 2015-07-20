

while read line; do
	if [ -d mutation_out ]; then
		rm -rf mutation_out
	fi
	if [ -f gstats.xml ]; then
		rm -f gstats.xml
	fi
	./Controller $line/config.cfg
	if [ -d $line/mutation_out ]; then
		rm -rf $line/mutation_out
	fi
	if [ -f $line/gstats.xml ]; then
		rm -f $line/gstats.xml
	fi
	cp -r mutation_out $line/mutation_out
	cp gstats.xml $line/gstats.xml
done < subjectList.txt

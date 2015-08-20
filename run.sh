if [ $# -gt 0 ]; then
	postfix=$1
	if [ $# -gt 1 ]; then
		skip=$2
		((skip=skip-1))
	else
		skip=0
	fi
else
	postfix=M
	skip=0
fi

while read line; do
	if [ $skip -gt 0 ]; then
		((skip=skip-1))
		continue
	fi
	echo $line
	if [ -d mutation_out ]; then
		rm -rf mutation_out
	fi
	if [ -f gstats.xml ]; then
		rm -f gstats.xml
	fi
	./Controller $line/config.cfg
	if [ -d $line/mutation_out_$postfix ]; then
		rm -rf $line/mutation_out_$postfix
	fi
	if [ -f $line/gstats_$postfix.xml ]; then
		rm -f $line/gstats_$postfix.xml
	fi
	cp -r mutation_out $line/mutation_out_$postfix
	cp gstats.xml $line/gstats_$postfix.xml
done < subjectList.txt

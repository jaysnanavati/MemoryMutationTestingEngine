listFile=subjectList.txt
cwd=`pwd`

if [ -f $listFile ]; then
	rm $listFile
fi

for n in `ls`; do
	if [ -d $n ]; then
		echo $cwd/$n >> $listFile
	fi
done

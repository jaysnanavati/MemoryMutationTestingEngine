#!/bin/bash 
GREP_STR="branch\s+[0-9]+\s+(taken\s+[0-9]+|never\sexecuted)"
source_dir=$1
output_dir=$2
file_only=$3

if [[ -z "$file_only" ]]; then 
 for i in $source_dir/*.gcov; do
    grep -E $GREP_STR $i >  $output_dir/$(basename $i).branches
 done;
else
 arr=( $(find $source_dir -name $file_only.c.gcov))
 if [[ ! -z ${arr[0]} ]]; then
 	grep -E $GREP_STR ${arr[0]} >  $output_dir/$(basename ${arr[0]}).branches
 fi
fi

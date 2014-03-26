#!/bin/sh
{

#This file verifies that all the source files are parsed successfully by
#our grammar, without having to run the program

#Add this functionallity in our program? to detect if a file did not parse and continue operation and report in the end..


directory=$1

find $directory -type f -name "*.c" -exec sh -c \
  'echo "\n\nchecking ::--> $(dirname $1)$(basename $1) <--" ; 
  cpp $(dirname $1)/$(basename $1) > $(dirname $1)
  txl c.Txl $(dirname $1)/$(basename $1) -o /dev/null' sh {} \;
}&>txlVerify.log
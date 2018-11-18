#!/bin/sh

#This shell script repeats the running of the Teep proof of concept (which writes some files) 
#and the testing of those files with the checkfile.py script. I let this run for hours to
#check that the program is working correctly. If either ./checkfile returns a failure
#or the POC gets stuck, we know it's not correct.
gcc -o poc poc.c -lpthread
if [ $? -ne 0 ]; then
	exit
fi
quitting=0 
rm file*
while [ $quitting -eq 0 ]; do
	./poc
	for filename in file*; do
		./checkfile.py $filename
		if [ $? -ne 0 ]; then
			echo "Failed!"
			quitting=1
			break
		fi
	done
done

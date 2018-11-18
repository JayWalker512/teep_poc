#!/bin/sh
quitting=0
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

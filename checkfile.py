#!/usr/bin/env python3
import sys

with open(sys.argv[1], "r") as f:
	inByte = "nada"
	nextChar = 'a'
	i = 0
	while inByte is not "":
		inByte = f.read(1)
		if inByte is not nextChar:
			if inByte is not "":
				print("Found an unexpected character: " + inByte + " @ " + str(i) + ", TEST FAILED")
				exit(1)

		if nextChar == 'b':
			nextChar = 'a'
		else:
			nextChar = 'b'


print("TEST PASSED")

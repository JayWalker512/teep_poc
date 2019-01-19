Teep Proof of Concept
---------------------

What is Teep? Teep is a parallel implementation of Tee, where writes all happen in parallel. This repository is not the source code for Teep, instead it is a simplified version of the code which Teep uses to parallelize writes. I wrote this proof of concept to check that my multithreaded code was correct and free of race conditions.

Compiling
---------

Compile with:

gcc -o poc poc.c -lpthread

Theory of operation
-------------------

There is only one buffer, which is filled before any threads start. Once it is filled, all the writer threads start and write the buffered data to their destinations in parallel. The last writer to finish is tasked with re-filling the buffer, and then signaling the other threads that they can start writing again. So the pipeline diagram looks like this:
```
+------+-+-+-+-+
|read  |X| |X| |
+------+-+-+-+-+
|write1| |X| |X|
+------+-+-+-+-+
|write2| |X| |X|
+------+-+-+-+-+
|writeN| |X| |X|
+------+-+-+-+-+
```
In this case only the writes are overlapped, but this should still provide some benefit.

Correctness checking
--------------------

When run with some N number of threads, poc will output a text files titled fileA, fileB, ... fileX for as many threads as are set to run in the source code (see 'numThreads' variable). Each of these files MUST contain a string following the pattern "ababababab"... as this is simulating the buffer being filled with 'a' then written, then filled with 'b' then written, and so on repeating until the required number of repetitions is met.

There are two scripts for checking this: *checkfile.py* is a Python3 script which verifies a file's contents follow the above described pattern. Use it like so:

```python3 checkfile.py fileToCheck```

The shell script *testloop.sh* will re-compile poc when run. Then in a loop it will run the poc and run *checkfile.py* on each output file produced. This loop will repeat infinitely until stopped. If you can run this script for several hours without error, it's a good indication that the implementation is working correctly.



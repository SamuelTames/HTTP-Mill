# HTTP Header Counting Challenge
November 23, 2015 - Sam Tames

# TLDR
* make
* generate a testfile with: ./mk_testfile.py > testfile.txt
* or use your own \r\n terminated headers.
* ./HeaderCounter testfile.txt
*  [README for matching/counting data structure used](header_counter_lib/README.md)

# Problem Statement
Please write a program in C that reads and analyzes the content of a file. The file in question contains any number of well-formed HTTP headers (rfc 2616) and their values in canonical form. Your program needs to read this file and determine the number of times a set of headers (chosen at compile time) occurs in the file.

For example, if your program tracked the 'Connection', 'Accept', and 'Content-Length' headers, and is run against the following file:
```
<file start>
Content-Length: 10
User-Agent: Test
Content-Length: 14
Accept: comedy
Content-Length: 100
Content-Encoding: gzip
Connection: close
User-Agent: Test
Accept: flash
User-Agent: Test1
Content-Length: 20
User-Agent: Test2
User-Agent: Test3
Accept: gzip
<file end>
```
it should indicate that Content-length was seen 4 times, Accept 3 times, etc. As a matter of practice, we recommend that your program try to match all the headers that a browser might be interested in.

The program does not need to do anything with the header value. Also, when thinking about this solution, please optimize for CPU, meaning that it's okay to take extra memory if you can find a good algorithm to reduce the processing.

# Building
* "make" in the root directory
* the program expects POSIX compliance
* the list of valid/countable headers is contained in headers.dat
* if header list changes, the header counter code must be regenerated (make handles this)
* modify HEADER_DIR in header_Counter_lib/ makefile to change data structure

# RFC 2616 interpretation
### grammar
* header names will not contain white-space
* header names will be terminated by ':'

### additional
* ASCII character space
* header values can be of any length
* headers are terminated with '\r''\n'
* it's suggested that header parsers accept a bare '\n' as a terminator
* this program accepts the strict interpretation of the terminal (this could be changed easily)

# Architecture
This problem can easily be solved through the following method in probably < 40 lines of code in main:

```
getdelim on ':'
  count parsed header name
getdelim on '\n' (check for \r as well)
repeat until EOF
```

This solution will under perform from the pauses in IO. (which may very well be okay depending on the use-case!)

* One solution would be to use reader threads enqueuing data to a processing thread pair. This can be hard to balance correctly. Either the IO threads will be sitting idle with full queues (CPU saturated, unable to keep queues from filling) or the CPU could be waiting for work. Moving away from a 1:1 producer:consumer model may help, but sharing as little as possible is wise. Lock contention could be of issue.
* Async IO could be another approach, but likely an unpleasant experience with POSIX APIs. (Something similar to Node.js)
* Yet another solution would be to take on the strategy of the simple solution: read the file, then process IO and scale that execution unit over "sufficient" threads to hopefully keep resources busy. This should work okay, but lacks the fine grain control that the next solution provides.
* The last approach would be use multiple threads, but allow each thread to read data or process data. A decision on what a thread should be doing could be guided by some constraints. ie {min,max}_io_threads, {min,max}_processing_threads. A thread check where it is needed after it completes its current task.

I chose the third solution to limit complexity.

# Solution
This solution does the following:
* It creates N threads (pthreads used as c11 thread support is iffy)
* each thread gets a file "chunk" of size FILESIZE / N
* each thread reads and counts headers in its chunk
* each thread ignores the beginning of its chunk up to the first header terminal found. It's likely that the start of the threads chunk splits a header creating header fragments.
* thread N-1 reads past its end until the first header terminal is found. That is, the previous thread handles the complete header that the next couldn't see.
* each thread stores its header data in its own copy of a HeaderCounter
* when the threads chunk is processed it merges its HeaderCounter with the global HeaderCounter from main. A mutex is used here. (atomic adds could be used inside the HeaderCounter merge)
* Threads join and the results are printed in the order that headers.dat contains.
* Using stdio buffered for reading. Using read(2) could be an option, but really would just be reinventing stdio buffering in this program.

This strategy when paired with the efficient data structure chosen(see [header_counter_lib/README.md](header_counter_lib/README.md)) to find header matches and hold the counts of each header allows for a fast solution and very low memory usage(cache friendly). (an increase in memory usage here would not lower CPU consumption)

# Things to consider
It may be beneficial for threads to look at smaller "chunks" of the file, but own multiple chunks. This could allow for more of the reads to occur closer together; attempting to get closer to a sequential read. (which are faster when read from traditional disk setups). Seen below with A vs B dividing into chunks each thread is responsible for. I suspect this is not worth pursuing though.
```
  .........................
A .     .     .     .     .
  .  1  .  2  .  3  .  4  .
  .........................
B .  .  .  .  .  .  .  .  .
  . 1. 2. 3. 4. 1. 2. 3. 4.
  .........................
```
### Why not mmap(2)?
mmap(2) can be useful if there is significant time spent with random reads/writes across a file. This problem given is simply a sequential read where the "file used as easily as memory" abstraction is not needed. Benchmarks show buffered fstream or read(2) to be as fast/faster[citation here] than mmap, as mmap must read the io and commit to memory just the same. mmap can also bring along a performance hit, as it attempts to keep the file in memory and result in page thrashing. (the file may not even fit in ram)

### Data structure to count headers
* see [header_counter_lib/README.md](header_counter_lib/README.md)
* perfect minimal hash table - an efficient, low memory usage, and cache friendly solution. (compile time header key-space allows optimizations)

# TODO
* thorough testing to reveal the inevitable bugs.
* basic tests that have been wrote involve generating some random valid headers and testing the sum of header counts against wc -l
* also tested against small edge-case files by hand
* if this was a real application; profile and optimize where needed if performance isn't met. (flush caches to better gauge IO performance)
* some program architecture redesign could be considered if needed.
* see if hinting to the kernel about file usage helps. ie posix_fadvice(2)
* maybe use setbuf(3) to tweak fstream io.
* explore faster hash functions
* cleanup src + docs
* take a better look at the build process
* look for integer overflow issues
* test the binary search hash counter implementation
* possibly add a count for "unknown" headers found

# Tools used
### clang/gcc

### valgrind
* no leaks
* mild cachegrind usage

### flint
* no errors

### scan-build
* no errors

### AddressSanitizer
* no errors (yet)

### perf + flamegraphs
* ftell appears to be a _mild_ addressable "hot spot" (...warm spot?)
* keeping track of file pos counters might eliminate ftell.

### mk_testfile.py
* generates file with headers with names taken from headers.dat and random values
* files in 4GB range process in ~6 real seconds with: i5 750 and crucial mx100 ssd

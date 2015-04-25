*** pre-alpha ***
(validation, cleanup, dynamic library, documentation, and examples: work in progress)



libsrt: Simple Real-Time library for the C programming language
===

libsrt is a C library that provides string, vector, tree, and map handling. It's been designed for avoiding explicit memory management, allowing safe and expressive code, while keeping high performance. It covers basic needs for writing high level applications in C without worrying about managing dynamic size data structures. It is also suitable for low level and hard real time applications, as *all* functions are predictable in both space and time.

Key points:

* Easy: write high-level-like C code. Write code faster and safer.
* Fast: using O(n)/O(log n)/O(1) state of the art algorithms.
* Useful: UTF-8 strings, vector, tree, and map structures.
* Efficient: space optimized, minimum allocation calls (heap and stack support).
* Compatible: OS-independent (e.g. built-in space-optimized UTF-8 support).
* Predictable: suitable for microcontrollers and hard real-time compliant code.
* Unicode: UTF-8 support for strings, including binary/raw data cases.

Generic advantages
===

* Ease of use
 * Use strings in a similar way to higher level languages.

* Space-optimized
 * Dynamic one-block linear addressing space.
 * Internal structures use indexes instead of pointers.

* Time-optimized
 * Buffer direct access
 * Preallocation hints (reducing memory allocation calls)
 * Heap and stack memory allocation support (s\*\_alloca)
 * Fast O(log(n)) algorithms run as fast as other O(1) implementations because of space-optimization (cache hit).

* Predictable (suitable for hard and soft real-time)
 * Predictable execution speed (O(1), O(n), and O(log n) algorithms).
 * Hard real-time: allocating maximum size (strings, vector, trees, map) will imply calling 0 or 1 times to the malloc() function (C library). Zero times if using the stack as memory or externally allocated memory pool, and one if using the heap. This is important because malloc() implementation has both memory and time overhead, as internally manages a pool of free memory (which could have O(1), O(log(n)), or even O(n), depending on the compiler provider C library implementation).
 * Soft real-time: logarithmic memory allocation: for 10^n new elements just log(n) calls to the malloc() function. This is not a guarantee for hard real time, but for soft real time (being careful could provide almost same results, except in the case of very poor malloc() implementation in the C library being used -not a problem with modern compilers-).

* RAM, ROM, and disk operation
 * Data structures can be stored in ROM memory.
 * Data structures are suitable for memory mapped operation, and disk store/restore (full when not using pointers but explicit data)

* Known edge case behavior
 * Allowing both "carefree code" and per-operation error check. I.e. memory errors and UTF8 format error can be checked after every operation.

* Implementation
 * Simple internal structures, with linear addressing. It allows to reduce memory allocation calls to the minimum (using the stack it is possible to avoid heap usage).

* Test-covered
 * Implementation kept as simple as possible/known.
 * No duplicated code for handling the different data structures: at logic level are the same, and differences are covered using a minimal abstraction for accessing the meta-information (string size, alloc size, flags, etc.).
 * Small code, suitable for static linking.
 * Coverage, profiling, and memory leak/overflow/uninitialized checks (Valgrind, GNU gprof -not fully automated into the build, yet-).

* Compatibility
 * C99 and C++ compatible
 * CPU-independent: endian-agnostic, aligned memory accesses.
 * E.g. GCC C and C++ (C89, C99, C++/C++11), TCC, CLANG C and C++, MS VS 2013 C and C++ compilers. Visual Studio 2013 "project" is provided just for running the test.


Generic disadvantages/limitations
===

* Double pointer usage: because of using just one allocation, operations require to address a double pointer, so in the case of reallocation the source pointer could be changed. A trivial solution for avoiding mistakes is to use the same variable for a given element, or using double pointers.

* Without the resizing heuristic one-by-one increment would be slow, as it makes to copy data content (with the heuristic the cost it is not only amortized, but much cheaper than allocating things one-by-one).

* Concurrent read-only operations is safe, but concurrent read/write must be protected by the user (e.g. using mutexes or spinlocks). That can be seen as a disadvantage or as a "feature" (it is faster).

* Not fully C89 compatible, unless following language extensions are available:
 * \_\_VA\_ARGS\_\_ macro
 * alloca()
 * type of bit-field in 'struct'
 * %S printf extension (only for unit testing)
 * Format functions (\*printf) rely on system C library, so be aware if you write multi-platform software before using compiler-specific extensions or targetting different C standards).

* Focused to reduce verbosity:
 * ss\_cat(&t, s1, ..., sN);
 * ss\_cat(&t, s1, s2, ss\_printf(&s3, "%i", cnt), ..., sN);
 * ss\_free(&s1, &s2, ..., &sN);
 * Expressive code without explicit memory handling.

* Focused to reduce and/or avoid errors, e.g.
 * If a string operation fails, the string is kept in the last successful state (e.g. ss\_cat(&a, b, huge\_string, other))
 * String operations always return valid strings, e.g.
		This is OK:
	  		ss_t *s = NULL;
			ss_cpy_c(&s, "a");
		Same behavior as:
			ss_t *s = ss_dup_c("a");
 * ss\_free(&s1, ..., &sN);  (no manual set to NULL is required)


String-specific advantages
===

* Unicode support
 * Internal representation is in UTF-8
 * Search and replace into UTF-8 data is supported
 * Full and fast Unicode lowercase/uppercase support without requiring "setlocale" nor hash tables.
* Binary data support
 * I.e. strings can have zeros in the middle
 * Search and replace into binary data is supported
* Efficient raw and Unicode (UTF-8) handling. Unicode size is tracked, so resulting operations with cached Unicode size, will keep that, keeping the O(1) for getting that information afterwards.
 * Find/search: O(n), one pass.
 * Replace: O(n), one pass. Worst case overhead is limited to a realloc and a copy of the part already processed.
 * Concatenation: O(n), one pass for multiple concatenation. I.e. Optimal concatenation of multiple elements require just one allocation, which is computed before the concatination. When concatenating ss\_t strings the allocation size compute time is O(1).
 * Resize: O(n) for worst case (when requiring reallocation for extra space. O(n) for resize giving as cut indicator the number of Unicode characters. O(1) for cutting raw data (bytes).
 * Case conversion: O(n), one pass, using the same input if case conversion requires no allocation over current string capacity. If resize is required, in order to keep O(n) complexity, the string is scanned for computing required size. After that, the conversion outputs to the secondary string. Before returning, the output string replaces the input, and the input becomes freed.
 * Avoid double copies for I/O (read access, write reserve)
 * Avoid re-scan (e.g. commands with offset for random access)
 * Transformation operations are supported in all dup/cpy/cat functions, in order to both increase expressiveness and avoid unnecessary copies (e.g. tolower, erase, replace, etc.). E.g. you can both convert to lower a string in the same container, or copy/concatenate to another container.
* Space-optimized
 * 5 byte overhead for strings below 507 bytes (4 bytes for the data structure, and one reserved for \0 ending)
 * (sizeof(size\_t) * 4 + 1) byte overhead for strings longer than 506 bytes (e.g. 17 bytes for a 32-bit CPU, 33 for 64-bit)
 * Data structure has no pointers, i.e. just one allocation is required for building a string. Or zero, if using the stack.
 * No additional memory allocation for search.
 * Extra memory allocation may be required for: UTF-8 uppercase/lowercase and replace.
 * Strings can grow from 0 bytes to ((size\_t)~0 - metainfo\_size)
* String operations
 * copy, cat, tolower/toupper, find, split, printf, cmp, etc.
 * allocation, buffer pre-reserve,
 * Raw binary content is allowed, including 0's.
 * "Wide char" and "C style" strings R/W interoperability support.
 * I/O helpers: buffer read, reserve space for async write
 * Aliasing suport, e.g. ss\_cat(&a, a) is valid

String-specific disadvantages/limitations
===

 * No reference counting support. Rationale: simplicity.

Vector-specific advantages
===

 * Variable-length concatenation and push functions.
 * Allow explicit size for allocation (8, 16, 32, 64 bits) with size-agnostic generic signed/unsigned functions (easier coding).

Vector-specific disadvantages/limitations
===


Tree-specific advantages
===

* Red-black tree implementation using linear memory pool: only 8 bytes per node overhead (31-bit * 2 indexes, one bit for the red/black flag), self-pack after delete, cache-friendy. E.g. a one million (10^6) node tree with 16 byte nodes (8 bytes for the overhead + 8 bytes for the user data) would requiere just 16MB of RAM: that's a fraction of memory used for per-node allocated memory trees (e.g. "typical" red-black tree for that example would require up to 8x more memory).
* Top-down implementation (insertion/deletion/traverse/search)
* Zero node allocation cost (via pre-allocation or amortization)
* O(1) allocation and deallocation
* O(log(n)) node insert/delete/search
* O(n) sorted node enumeration (amortized O(n log(n))). Tree traversal provided cases: preorder, inorder, postorder.
* O(n) unsorted node enumeration (faster than the sorted case)

Tree-specific disadvantages/limitations
===

* 

Map-specific advantages
===

 * Abstraction over the tree implementation, with same benefits, e.g. one million 32 bit key, 32 bit value map will take just 16MB of memory (16 bytes per element).
 * Keys: integer (8, 16, 32, 64 bits) and string (ss\_t)
 * Values: integer (8, 16, 32, 64 bits), string (ss\_t), and pointer
 * O(1) for allocation
 * O(1) for deleting maps without strings (one or zero calls to 'free' C function)
 * O(n) for deleting maps with strings (n + one or zero calls to 'free' C function)
 * O(log(n)) insert, delete
 * O(n) sorted enumeration (amortized O(n log(n)))
 * O(n) unsorted enumeration (faster than the sorted case)

Map-specific disadvantages/limitations
===

*

License
===

Copyright (c) 2015, F. Aragon. All rights reserved.
Released under the The BSD 3-Clause License (see the doc/LICENSE file included).

Contact
===

email: faragon.github (GMail account, add @gmail.com)

Other
===

Changelog
---

v0.0 (20150421) *pre-alpha*

"to do" list
---

See doc/todo.md file.


Acknowledgements and references
---

See doc/references.md file for acknowledgements and references.




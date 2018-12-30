Acknowledgements
===

* RB tree implementation based on Julienne Walker`s red-black tree insert and delete top down algorithms (beautiful)
  * http://www.eternallyconfuzzled.com/tuts/datastructures/jsw\_tut\_rbtree.aspx
* Rabin-Karp search algorithm
  * http://en.wikipedia.org/wiki/Rabin%E2%80%93Karp_algorithm
* 7-bit parallel case conversions learned from Paul Hsieh examples
  * http://www.azillionmonkeys.com/qed/asmexample.html

References (recommended lectures and code review)
===

Generic data structures
---

* libulz
  * https://github.com/rofl0r/libulz
* GDSL (reentrant): http://home.gna.org/gdsl/
* Gnulib: https://www.gnu.org/software/gnulib/
  * http://git.savannah.gnu.org/gitweb/?p=gnulib.git;a=tree;f=lib;h=7f936d3be56da0cbeb950256c60cc7bb900826f4;hb=HEAD
* Apache APR
  * Includes iconv, snprintf, etc. Very interesting.
  * https://apr.apache.org/download.cgi
  * atomic: atomic increments
  * dso: dynamic library
  * encoding: ASCII <-> EBCDIC, escape/unscape URL, hex, escape/unscape "entity" (ML)
  * file\_io: copy dir, seek, r/w, open, mktemp, dup, stat
  * locks: spinlock, mutex
  * memory: memory pool (Win32-only)
  * misc: utf8, charset, rand, error codes, env
  * mmap: map file on memory
  * network_io: sockets
  * passwd:
  * poll: fd poll (unix, os2)
  * random: random + sha256/384/512
  * shmem: shared memory IPC
  * strings: strnatcmp, strtok, fnmatch, snprintf, strings, cpystrn (no full Unicode support)
  * support: 3KB. waitio (Unix fd poll)
  * tables: hash table, skip list, memory leak detection helpers
  * test: unit tests
  * threadproc: thread, process, and signal utilities
  * time: time utils
  * tools: test generator helper
  * user: userinfo, groupinfo:
* GLib
  * https://developer.gnome.org/glib/stable/glib-String-Utility-Functions.html
  * Inspected: g_strnfill, g_str_has_prefix, g_str_to_ascii, g_str_is_ascii, g_ascii_islower, ...
* SGLIB
  * http://sglib.sourceforge.net/
  * Simple Generic Library for C
  * Faster than glib
  * BSD-like
* Attractive Chaos klib
  * Generic containers using C macros
  * https://github.com/attractivechaos/klib
  * http://attractivechaos.wordpress.com/2008/10/07/another-look-at-my-old-benchmark/
  * https://attractivechaos.wordpress.com/2008/10/13/the-rdehash_map-hash-library/
* clibutils
  * http://sourceforge.net/projects/clibutils/
  * https://github.com/rustyrussell/ccan/tree/master/junkcode/dongre.avinash%40gmail.com-clibutils
* liblfds
  * http://liblfds.org/
* BSD
  * http://openbsd.cs.toronto.edu/cgi-bin/cvsweb/src/sys/sys/queue.h
  * http://openbsd.cs.toronto.edu/cgi-bin/cvsweb/src/sys/sys/tree.h
* sparsehash
  * https://code.google.com/p/sparsehash/?redir=1
* Cliff Click`s hash table
  * http://preshing.com/20130605/the-worlds-simplest-lock-free-hash-table/
* uSTL
  * https://msharov.github.io/ustl/
  * https://github.com/msharov/ustl
* libwayne (Wayne`s Little Data Structures and Algorithms Library)
  * http://www.cs.toronto.edu/~wayne/libwayne/
* klib
  * http://blog.qoid.us/2014/04/10/klib.html
* qlib
  * https://github.com/wolkykim/qlibc
* libxc (Extended C Library)
  * http://sourceforge.net/projects/libxc/
  * It has threaded support (POSIX-only). Doxygen doc.
  * xvector: 20+4KB. Growable array (shrinking not supported), del/insert use memmove (i.e. slow)
  * xtree: 9+1KB. Not sorted trees, but trivial generic tree.
  * xstring: 34+1KB. Without Unicode support. Slow search. Very limited functionality.
  * xsparse: 13+1KB. Naive map-like array. Uses xdict.
  * xsock: 19+1KB. TCP sockets wrapper.
  * xshare: 8+2KB. Dynamic library handling wrapper.
  * xsarray: 12+1KB. Sorted array. Bult over xvector.
  * xproc: Empty. Shows plans for thread wrapper implementation.
  * xmalloc: 10+2KB. Wrapper for malloc.
  * xfile: 15+1KB. Some file utilities. A bit strange.
  * xerror: 5+4KB. Error reporting (log-like).
  * xdict: 19+1KB. Map implementation ("associative arrays"). Naive implementation.
  * xcrypto: 43KB. Random numbers and SHA256 hashes.
  * xcfg: 24+1KB. Configuration file handling (read/write of key-value elements).
* libshl (not reviewed yet)
  * https://github.com/dvdhrm/libshl
* libscl (not reviewed yet)
  * http://sourceforge.net/projects/libscl/
* Compact libc implementations
  * musl: https://www.musl-libc.org/
  * uClibc: https://uclibc.org/
  * diet libc: https://www.fefe.de/dietlibc/
* The Art of Computer Programming (Donald Knuth): not used for this, but it is always a nice mention.

Strings
---

* SDS (Salvatore Sanfilippo, antirez)
  * README.md structure.
  * Equivalent sdsfromlonglong(), sdstrim() functionality.
* strbuf (Git project)
  * Equivalent strbuf\_grow(), strbuf\_split\_\*() functionality.
  * Documentation structure (https://github.com/git/git/blob/master/Documentation/technical/api-strbuf.txt)
* string library comparison
  * http://www.and.org/vstr/comparison
  * http://bstring.sourceforge.net/features.html

Unicode / character handling
---

* Unicode case/folding: http://ftp.unicode.org/Public/UNIDATA/CaseFolding.txt
* Unicode table: http://unicode-table.com/en/

Sorting
---

* Generic
  * https://stackoverflow.com/questions/10604693/fastest-sorting-technique
*  Timsort
  * https://en.wikipedia.org/wiki/Timsort
  * http://bugs.java.com/bugdatabase/view_bug.do?bug_id=6804124

Hash tables
---

* uthash
  * https://troydhanson.github.io/uthash/
* Malte Skarupte blog posts
  * https://probablydance.com/2017/02/26/i-wrote-the-fastest-hashtable/
  * https://probablydance.com/2018/05/28/a-new-fast-hash-table-in-response-to-googles-new-fast-hash-table/
  * https://probablydance.com/2018/06/16/fibonacci-hashing-the-optimization-that-the-world-forgot-or-a-better-alternative-to-integer-modulo/
  * https://github.com/skarupke/flat_hash_map
* Misc
  * https://research.cs.vt.edu/AVresearch/hashing/
  * https://aras-p.info/blog/2016/08/02/Hash-Functions-all-the-way-down/
  * https://rcoh.me/posts/hash-map-analysis/
  * http://www.idryman.org/blog/2017/05/03/writing-a-damn-fast-hash-table-with-tiny-memory-footprints/
  * http://www.ilikebigbits.com/2016_08_28_hash_table.html
* Benchmarks
  * http://incise.org/hash-table-benchmarks.html
  * https://tessil.github.io/2016/08/29/benchmark-hopscotch-map.html
  * https://lonewolfer.wordpress.com/2015/02/07/benchmarking-string-hash-tables/
  * https://github.com/nfergu/hashtableperf

Trees
---

* StackOverflow stuff
  * http://stackoverflow.com/questions/647537/b-tree-faster-than-avl-or-redblack-tree
* Julienne Walker web site
  * http://www.eternallyconfuzzled.com
* Wikipedia
  * http://en.wikipedia.org/wiki/Red%E2%80%93black_tree
  * http://en.wikipedia.org/wiki/B-tree
  * http://en.wikipedia.org/wiki/Tree_traversal
  * http://en.wikipedia.org/wiki/Left-leaning_red%E2%80%93black_tree
  * http://en.wikipedia.org/wiki/2%E2%80%933_tree
  * http://en.wikipedia.org/wiki/2%E2%80%933%E2%80%934_tree
* Misc
  * http://algs4.cs.princeton.edu/33balanced/RedBlackBST.java.html
  * http://www.canonware.com/rb/
  * http://www.teachsolaisgames.com/articles/balanced_left_leaning.html
  * http://25thandclement.com/~william/projects/llrb.h.html
  * http://t-t-travails.blogspot.com/2010/04/red-black-trees-revisited.html

Memory allocation
---

* http://stackoverflow.com/questions/282926/time-complexity-of-memory-allocation
* http://www.gii.upv.es/tlsf/
* http://www.design-reuse.com/articles/25090/dynamic-memory-allocation-fragmentation-c.htm

Node cache size impact
---

* Effect of Node Size on the Performance of CacheConscious B+trees (Richard A. Hankins, Jignesh M. Patel)

String Matching
---

* Simple Real-Time Constant-Space String Matching (Dany Breslauer, Roberto Grossi, Filippo Mignosi)
* Text Searching Algorithms vol II: Backward String Matching (Borivoj Melichar)

Predictability
---

* Building Timing Predictable Embedded Systems (Philip Axer, Rolf Ernst, Heiko Falk, ...) [2012]
* Static Timing Analysis for Hard Real-Time Systems (Reinhard Wilhelm, Sebastian Altmeyer, ...)
* Timing-Predictable Memory Allocation In Hard Real-Time Systems (Jörg Herter) [2014]

Data compression
---

* Real-time LZ77 data compression
  * lzo http://www.oberhumer.com/opensource/lzo/
  * libslz http://www.libslz.org/
  * lz4 https://github.com/lz4/lz4

C language
---

* C preprocessor
  * var args comp.lang.c FAQ http://www.lysator.liu.se/c/c-faq/c-7.html
  * Count arguments: http://stackoverflow.com/questions/2124339/c-preprocessor-va-args-number-of-arguments
* C99 standard
  * ISO/IEC 9899:TC3 Draft 20070907 http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1256.pdf

Misc
---

* suckless
  * http://suckless.org/rocks
  * http://suckless.org/sucks


"to do" list
===

Short-term
---
* copy behavior: cut on size limit (stack storage and size_t limits)
* size/len incoherences (e.g. get_len / get_size)
* add overflow checks, using "s_size_t_overflow"
* Add st_shl and st_shr for shifting elements on a vector (without real data shift).
* Write some simple examples
* Add built-in funcs: int __builtin_popcount/__popcnt, __builtin_parity / _popcnt(a) & 1;
* Add bit vectors in some way (a subset of bitset, e.g. just for counting)
* Add a (k, void) new types to smap (e.g for cases of existance check)

Mid-term
---

* Dynamic disk/RAM (mmap) allocators
 * ..._allocd: new allocators supporting dynamic memory mapping. I.e. instead of having a fixed-size memory mapped area, allow to map dynamically (for that 'realloc' callbacks should be added).
* Think about adding more types
 * stable: read-only "frozen" tree/map
* Vector enhancements
 * Sort
* Map enhancements
 * Apply function to map: for all, for range, etc.
* Tree enhancements
 * Make last-inserted nodes cache, in order to speed-up delete.
 * Iterator: In addition to the callback, add an iterator for traversal.
 * min, max, range query
 * Compare, substract, add.
* String enhancements
 * Store string search hash at string end (mark it with flag)
 * Search multiple targets on string keeping with O(m * n) worst search time (not O(n^2), but cheap one-pass)
 * Cache Rabin-Karpin hashes for amortized cost when repeating search with same target ("needle").
 * Add SSE 4.2 SIMD intrinsics for increasing from 1GB/s up to the saturation of the memory bus (e.g. 10GB/s per core at 3GHz) \-not 100% sure if that much will be possible, but I have some ideas\-
 * Quote strings (e.g. like in sds)
 * String analysis (naive Bayes, weight/entropy)
 * Check if some important string functionality is missing.
 * Unicode: to_title, fold_case and normalize. http://www.boost.org/doc/libs/1_51_0/libs/locale/doc/html/conversions.html http://en.wikipedia.org/wiki/Capitalization http://ftp.unicode.org/Public/UNIDATA/CaseFolding.txt
* Other
 * Bindings for other languages
* Add some simple data compression (e.g. 4KB LZW or 32KB LZ77 + Huffman coding)
* Win32-specific optimizations
 * Study if using VirtualAlloc could be used as faster realloc.
* Write examples
 * Key-value database: trivial case (string-string key-value) and zero alloc case (key/value fitting on fixed-size node)

Long-term (never)
---

* Time/date
* C++ wrapper
* Add UTF8 regexp support

QA
===

Short-term
---

* more synthetic description, simplify it (avoid unnecesary detail)
* review function per functtion.
* write readme.md and documentation (manual pages, Doxygen)
* Ensure: if can not no realloc memory, string is kept untouched.
* List C library dependencies

Mid-term
---

* Tests
 * ADD tests for every operation related to unicode caching.
 *  Check case unicode size (add tests: e.g. erasing with overflow, etc.)
 * aliasing: stack, lower/middle/upper case checks, etc.
 * sstring accessors (ss_get_buffer, ...)
 * tests using stack space
 * add tests covering expansion between small/medium/full strings
 * add checks for all corner cases (!)
 * reentrancy: 1) write size afterwards, 2) in case of buffer switch, point those cases in the "behavior" section.


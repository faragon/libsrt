"to do" list
===

Short-term
---

* Security: check size_t overflow on every function (e.g. off + size -> overflow)
* Add doc: xx_empty(), and other already implemented functions
* Tests
 * all tests using both stack and heap space
 * add grow from NULL tests
 * add tests for every operation related to Unicode caching.
 * add tests covering expansion between small/medium/full strings
 * check case Unicode size (add tests: e.g. erasing with overflow, etc.)
 * aliasing: stack, lower/middle/upper case checks, etc.
 * add checks for all corner cases (!)
 * reentrancy: 1) write size afterwards, 2) in case of buffer switch, point those cases in the "behavior" section.

Long-term
---

* Dynamic disk/RAM (mmap) allocators
 * allocd: new allocators supporting dynamic memory mapping. I.e. instead of having a fixed-size memory mapped area, allow to map dynamically (for that 'realloc' callbacks should be added).
* Read-only types
 * Optimized read-only tree, map, vector, and sorted vector
* Vector enhancements
 * Sort, st_shl and st_shr (shifting elements on a vector, without real data shift)
* Map enhancements
 * Apply function to map: for all, for range, etc.
 * Add a (k=\*, void) new types to smap (e.g for cases of existence check)
* Tree enhancements
 * Make last-inserted nodes cache, in order to speed-up delete.
 * Iterator: In addition to the callback, add an iterator for traversal.
 * min, max, range query
 * Compare, subtract, add.
* String enhancements
 * Store string search hash at string end (mark it with flag)
 * Search multiple targets on string keeping with O(m * n) worst search time (not O(n^2), but cheap one-pass)
 * Cache Rabin-Karpin hashes for amortized cost when repeating search with same target ("needle").
 * Add SSE 4.2 SIMD intrinsics for increasing from 1GB/s up to the saturation of the memory bus (e.g. 10GB/s per core at 3GHz) \-not 100% sure if that much will be possible, but I have some ideas\-
 * Quote strings (e.g. like in sds)
 * String analysis (naive Bayes, weight/entropy)
 * Unicode: to_title, fold_case and normalize. http://www.boost.org/doc/libs/1_51_0/libs/locale/doc/html/conversions.html http://en.wikipedia.org/wiki/Capitalization http://ftp.unicode.org/Public/UNIDATA/CaseFolding.txt
* Other
 * Bindings for other languages
* Write examples
 * Key-value database: string/int-string/int key-value), zero alloc case (key/value fitting on fixed-size node), KKV (key-key-value)

Never (?)
---

* Time/date
* C++ wrapper
* Add UTF8 regexp support
* Win32-specific optimizations
 * Study if using VirtualAlloc could be used as faster realloc.


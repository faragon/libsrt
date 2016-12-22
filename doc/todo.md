"to do" list
===

Short-term
---

* Security
 * check size\_t overflow on every function (e.g. off + size -> overflow)
* Debug
 * Add sm\_t, sms\_t, etc. prettyprinting
* Tests
 * All tests using both stack and heap space
 * Add grow from NULL tests for all cases
 * Add tests for every operation related to Unicode caching.
 * Add tests covering expansion between small/medium/full strings
 * Check case Unicode size (add tests: e.g. erasing with overflow, etc.)
 * Aliasing: stack, lower/middle/upper case checks, etc.
 * Add checks for all corner cases
 * Reentrancy: 1) write size afterwards, 2) in case of buffer switch, point those cases in the "behavior" section.

Mid-term
---

* Tree enhancements
 * Make last-inserted nodes cache, in order to speed-up delete.
* String enhancements
 * Quote strings (e.g. like in sds)

Never (?)
---

* Dynamic disk/RAM (mmap) allocators
 * allocd: new allocators supporting dynamic memory mapping. I.e. instead of having a fixed-size memory mapped area, allow to map dynamically (for that 'realloc' callbacks should be added).
* String enhancements
 * Store string search hash at string end (mark it with flag)
 * Search multiple targets on string keeping with O(m * n) worst search time (not O(n^2), but cheap one-pass)
 * Cache Rabin-Karpin hashes for amortized cost when repeating search with same target ("needle").
 * Add SSE 4.2 SIMD intrinsics for increasing from 1GB/s up to the saturation of the memory bus (e.g. 10GB/s per core at 3GHz) \-not 100% sure if that much will be possible, but I have some ideas\-
 * Unicode: to_title, fold_case and normalize. http://www.boost.org/doc/libs/1_51_0/libs/locale/doc/html/conversions.html http://en.wikipedia.org/wiki/Capitalization http://ftp.unicode.org/Public/UNIDATA/CaseFolding.txt
* Vector enhancements
 * st\_shl and st\_shr (shifting elements on a vector, without real data shift)
* Time/date
* Add UTF8 regexp support
* Win32-specific optimizations
 * Study if using VirtualAlloc/VirtualAllocEx could be used as faster realloc.


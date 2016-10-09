"to do" list
===

Short-term
---

* Security: check size\_t overflow on every function (e.g. off + size -> overflow)
* Tests
 * sb_clear, sb_shrink
 * sdm_dup, sdm_reset, sdm_set_routing, sdm_s_route, sdm_shrink
 * sm_reset, sm_grow, sm_reserve, sm_cpy, sm_ii32_inc, sm_uu32_inc, sm_ii_inc, sm_si_inc, sm_i_delete, sm_s_delete, sm_enum, sm_enum_r, sm_inorder_enum
 * ss_set_size, ss_real_off, ss_alloc_errors, ss_encoding_errors, ss_clear_errors, ss_cpy_wn, ss_enc_b64, ss_enc_hex, ss_enc_HEX, ss_enc_esc_json, ss_enc_esc_xml, ss_enc_esc_url, ss_enc_esc_dquote, ss_enc_esc_squote, ss_dec_b64, ss_dec_hex, ss_dec_esc_json, ss_dec_esc_xml, ss_dec_esc_url, ss_dec_esc_dquote, ss_dec_esc_squote, ss_findb, ss_findbm, ss_findc, ss_findnb, ss_find_cn, ss_findr, ss_findrb, ss_findrbm, ss_findrc, ss_findrnb, ss_findr_cn
 * st_alloca, st_grow, st_reserve, st_shrink, st_size, st_set_size, st_len, st_dup, st_enum, st_enum_r
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
 * st\_shl and st\_shr (shifting elements on a vector, without real data shift)
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
* C++ wrapper
 * Benefits: stack allocation for complex structures and memory savings because of pointer-less internal organization

Never (?)
---

* Time/date
* Add UTF8 regexp support
* Win32-specific optimizations
 * Study if using VirtualAlloc/VirtualAllocEx could be used as faster realloc.


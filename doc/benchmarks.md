Benchmarks overview
===

* 'bench' executable is built using 'make'
* Memory usage retrieved manually with htop (that's the reason for using MiB instead of MB as unit)
* Memory usage does not reflect real per-element allocation, because of resizing heuristics, both for libsrt (C/C++) and STL (C++). E.g. a 10^6 byte vector shows using 3.5 MiB (~3.6 bytes per element) while a 10^8 would take 100 MiB (closer to the expected 1 byte per element).
* Benchmark time precision limited to clock\_gettime() resolution on Linux
* Some functions are not yet optimized (e.g. vector)

Notes on libsrt space and time complexity
===

* libsrt strings (ss\_t)
 * Overhead for strings below 256 bytes: 5 bytes (including space for terminator for ensuring ss\_c() -equivalent to .c\_str() in C++- will work properly always)
 * Overhead for strings >= 256 bytes: 1 + 5 * sizeof(size\_t) bytes, i.e. 21 bytes in 32-bit mode, and 41 bytes for 64-bit (including 1 byte reserved for optional terminator)
 * Time complexity for concatenation: O(n)  -fast, allowing multiple concatenation with just one logical resize-
 * Time complexity for string search: O(n)  -fast, using Rabin-Karp algorithm with dynamic hash function change-
* libsrt vectors (sv\_t)
 * Overhead: 5 * sizeof(size\_t) bytes
 * Time complexity for sort: O(n) for 8-bit (counting sort), O(n log n) for 16/32/64 bit elements (MSD binary radix sort), and same as provided C library qsort() function for generic elements
* libsrt maps (sm\_t)
 * Overhead (global): 6 * sizeof(size\_t) bytes
 * Overhead (per map element): 8 bytes (31 bits x 2 for tree left/right, 1 bit for red/black, 1 bit unused)
 * Special overhead: for the case of string maps, strings up to 19 bytes in length are stored in the node (up to 19 x 2, in the case of string-string map), without requiring extra heap allocation. Because of that, when strings with length >= 20, 2 * 20 bytes (32-bit mode) or 2 * 16 bytes (64-bit mode) is wasted per node. The overhead is small, as in every case the memory usage of libsrt strings is under C++ std::map<std::string, std::string> memory usage (in the case of strings below 20 bytes, the difference is abysmal).
 * Time complexity for insert, search, delete: O(log n)
 * Time for cleanup ("free"/"delete"): O(1) for sets not having string elements, O(n) when having string elements
* libsrt sets (sms\_t)
 * Similar to the map case, but with smaller nodes (key-only)
 * Time complexity for insert, search, delete: O(log n)
 * Time for cleanup ("free"/"delete"): O(1) for maps not having string elements, O(n) when having string elements
* libsrt bitsets (sb\_t)
 * Overhead: 5 * sizeof(size\_t) bytes (implemented internally over sv\_t)
 * Time complexity for set/clear: O(1)
 * Time complexity for population count ("popcount"): O(1)  -because of tracking set/clear operations, avoiding the need of counting on every call-

Notes on STL (C++) space and time complexity
===

* STL strings
 * Depending on implementation, it could have optimization for short strings (<= 16 bytes) or not
 * Non-optimized short strings or strings over 16 bytes, by default require heap allocation, even if the instance is allocated in the stack (in C++11 can be forced using stack allocation, with explicit allocator, but it is not the default behavior)
 * Time complexity for concatenation: O(n)  -fast-
 * Time complexity for string search: O(n)  -fast for most cases, but slow for corner cases like e.g. needle = '1111111112', haystack = '111111111111111111111111111111111111112'-)
* STL vectors
 * No default stack allocation
 * Time complexity for sort: O(n log n)
* STL maps
 * Overhead per map element is notable, because the red-black tree implementation
 * Time complexity for insert, search, delete: O(log n)
 * Time for cleanup ("free"/"delete"): O(1) for maps not having string elements, O(n) when having string elements
* STL sets
 * Similar to STL map
 * Time complexity for insert, search, delete: O(log n)
 * Time for cleanup ("free"/"delete"): O(1) for sets not having string elements, O(n) when having string elements
* STL hash-maps
 * Faster than maps for small elements (8 to 64 bit integers)
 * Slower than maps for big elements, e.g. strings
 * Time complexity for insert, search, delete: O(n)  -O(1) for in the case ideal hash having 0 collisions-
 * Time for cleanup ("free"/"delete"): O(1) if for hash-maps not having string elements, O(n) when having string elements
* STL bitsets
 * Require max size on compile time
 * Allows stack allocation
 * Time complexity for set/clear: O(1)
 * Time complexity for population count ("popcount"): O(n)

64-bit g++ 5.2.1 (x86\_64, Linux Ubuntu 15.10 x86\_64)
===

CPU: Intel Core i5-3330 @3GHz with 6MB L3 cache

* Insert or process 1000000 elements, cleanup

| Test | Insert count | Memory (MiB) | Execution time (s) |
|:---:|:---:|:---:|:---:|
| libsrt\_map\_ii32 | 1000000 | 18.496 | 0.390289 |
| cxx\_map\_ii32 | 1000000 | 49.476 | 0.255779 |
| cxx\_umap\_ii32 | 1000000 | 42.100 | 0.087814 |
| libsrt\_map\_ii64 | 1000000 | 27.520 | 0.330523 |
| cxx\_map\_ii64 | 1000000 | 65.248 | 0.292752 |
| cxx\_umap\_ii64 | 1000000 | 41.988 | 0.090005 |
| libsrt\_map\_s16 | 1000000 | 58.716 | 1.095077 |
| cxx\_map\_s16 | 1000000 | 185 | 0.710997 |
| cxx\_umap\_s16 | 1000000 | 178 | 1.132734 |
| libsrt\_map\_s64 | 1000000 | 209 | 1.365531 |
| cxx\_map\_s64 | 1000000 | 262 | 0.851561 |
| cxx\_umap\_s64 | 1000000 | 254 | 1.262276 |
| libsrt\_set\_i32 | 1000000 | 15.520 | 0.316496 |
| cxx\_set\_i32 | 1000000 | 49.476 | 0.245601 |
| libsrt\_set\_i64 | 1000000 | 19.236 | 0.359966 |
| cxx\_set\_i64 | 1000000 | 49.408 | 0.249820 |
| libsrt\_set\_s16 | 1000000 | 35.024 | 1.001737 |
| cxx\_set\_s16 | 1000000 | 109 | 0.555505 |
| libsrt\_set\_s64 | 1000000 | 109 | 1.226219 |
| cxx\_set\_s64 | 1000000 | 155 | 0.707712 |
| libsrt\_vector\_i8 | 1000000 | 3.764 | 0.035944 |
| cxx\_vector\_i8 | 1000000 | 3.708 | 0.001039 |
| libsrt\_vector\_i16 | 1000000 | 4.660 | 0.010058 |
| cxx\_vector\_i16 | 1000000 | 4.780 | 0.002074 |
| libsrt\_vector\_i32 | 1000000 | 6.704 | 0.010038 |
| cxx\_vector\_i32 | 1000000 | 6.812 | 0.002800 |
| libsrt\_vector\_i64 | 1000000 | 10.532 | 0.010077 |
| cxx\_vector\_i64 | 1000000 | 10.752 | 0.003686 |
| libsrt\_vector\_gen | 1000000 | 35.740 | 0.022060 |
| cxx\_vector\_gen | 1000000 | 34.368 | 0.012993 |
| libsrt\_string\_search\_easymatch\_long\_1a | 1000000 | - | 0.164118 |
| c\_string\_search\_easymatch\_long\_1a | 1000000 | - | 0.028138 |
| cxx\_string\_search\_easymatch\_long\_1a | 1000000 | - | 0.434457 |
| libsrt\_string\_search\_easymatch\_long\_1b | 1000000 | - | 0.192165 |
| c\_string\_search\_easymatch\_long\_1b | 1000000 | - | 0.050272 |
| cxx\_string\_search\_easymatch\_long\_1b | 1000000 | - | 0.318541 |
| libsrt\_string\_search\_easymatch\_long\_2a | 1000000 | - | 0.033841 |
| c\_string\_search\_easymatch\_long\_2a | 1000000 | - | 0.032094 |
| cxx\_string\_search\_easymatch\_long\_2a | 1000000 | - | 0.363062 |
| libsrt\_string\_search\_easymatch\_long\_2b | 1000000 | - | 0.277023 |
| c\_string\_search\_easymatch\_long\_2b | 1000000 | - | 0.407452 |
| cxx\_string\_search\_easymatch\_long\_2b | 1000000 | - | 0.623150 |
| libsrt\_string\_search\_hardmatch\_long\_1a | 1000000 | - | 0.144832 |
| c\_string\_search\_hardmatch\_long\_1a | 1000000 | - | 1.212318 |
| cxx\_string\_search\_hardmatch\_long\_1a | 1000000 | - | 1.209771 |
| libsrt\_string\_search\_hardmatch\_long\_1b | 1000000 | - | 0.144835 |
| c\_string\_search\_hardmatch\_long\_1b | 1000000 | - | 0.396852 |
| cxx\_string\_search\_hardmatch\_long\_1b | 1000000 | - | 1.188262 |
| libsrt\_string\_search\_hardmatch\_short\_1a | 1000000 | - | 0.037939 |
| c\_string\_search\_hardmatch\_short\_1a | 1000000 | - | 0.081795 |
| cxx\_string\_search\_hardmatch\_short\_1a | 1000000 | - | 0.079648 |
| libsrt\_string\_search\_hardmatch\_short\_1b | 1000000 | - | 0.039186 |
| c\_string\_search\_hardmatch\_short\_1b | 1000000 | - | 0.037638 |
| cxx\_string\_search\_hardmatch\_short\_1b | 1000000 | - | 0.126140 |
| libsrt\_string\_search\_hardmatch\_long\_2 | 1000000 | - | 0.219069 |
| c\_string\_search\_hardmatch\_long\_2 | 1000000 | - | 0.017608 |
| cxx\_string\_search\_hardmatch\_long\_2 | 1000000 | - | 0.360596 |
| libsrt\_string\_search\_hardmatch\_long\_3 | 1000000 | - | 0.811728 |
| c\_string\_search\_hardmatch\_long\_3 | 1000000 | - | 15.678417 |
| cxx\_string\_search\_hardmatch\_long\_3 | 1000000 | - | 19.249023 |
| c\_string\_loweruppercase\_ascii | 1000000 | - | 0.580960 |
| cxx\_string\_loweruppercase\_ascii | 1000000 | - | 0.158654 |
| c\_string\_loweruppercase\_utf8 | 1000000 | - | 0.581038 |
| cxx\_string\_loweruppercase\_utf8 | 1000000 | - | 0.158929 |
| libsrt\_bitset | 1000000 | 1.820 | 0.003663 |
| cxx\_bitset | 1000000 | 1.848 | 0.001785 |
| libsrt\_bitset\_popcount100 | 1000000 | 1.760 | 0.007338 |
| cxx\_bitset\_popcount100 | 1000000 | 1.896 | 0.013845 |
| libsrt\_bitset\_popcount10000 | 1000000 | 1.760 | 0.006733 |
| cxx\_bitset\_popcount10000 | 1000000 | 1.896 | 0.576769 |
| libsrt\_string\_cat | 1000000 | - | 0.659209 |
| c\_string\_cat | 1000000 | - | 3.496450 |
| cxx\_string\_cat | 1000000 | - | 0.376064 |

* Insert 1000000 elements, read all elements 10 times, cleanup

| Test | Insert count | Memory (MiB) | Execution time (s) |
|:---:|:---:|:---:|:---:|
| libsrt\_map\_ii32 | 1000000 | 18.496 | 1.768320 |
| cxx\_map\_ii32 | 1000000 | 49.476 | 1.571061 |
| cxx\_umap\_ii32 | 1000000 | 42.100 | 0.278534 |
| libsrt\_map\_ii64 | 1000000 | 27.520 | 1.703929 |
| cxx\_map\_ii64 | 1000000 | 65.248 | 1.831483 |
| cxx\_umap\_ii64 | 1000000 | 41.988 | 0.278806 |
| libsrt\_map\_s16 | 1000000 | 58.716 | 6.621592 |
| cxx\_map\_s16 | 1000000 | 185 | 4.658081 |
| cxx\_umap\_s16 | 1000000 | 178 | 5.680590 |
| libsrt\_map\_s64 | 1000000 | 209 | 8.916005 |
| cxx\_map\_s64 | 1000000 | 262 | 5.877650 |
| cxx\_umap\_s64 | 1000000 | 254 | 6.592078 |
| libsrt\_set\_i32 | 1000000 | 15.520 | 1.645501 |
| cxx\_set\_i32 | 1000000 | 49.476 | 1.480438 |
| libsrt\_set\_i64 | 1000000 | 19.236 | 1.796947 |
| cxx\_set\_i64 | 1000000 | 49.408 | 1.589099 |
| libsrt\_set\_s16 | 1000000 | 35.024 | 6.685535 |
| cxx\_set\_s16 | 1000000 | 109 | 4.177632 |
| libsrt\_set\_s64 | 1000000 | 109 | 8.631935 |
| cxx\_set\_s64 | 1000000 | 155 | 5.619492 |
| libsrt\_vector\_i8 | 1000000 | 3.764 | 0.064108 |
| cxx\_vector\_i8 | 1000000 | 3.708 | 0.007270 |
| libsrt\_vector\_i16 | 1000000 | 4.660 | 0.038234 |
| cxx\_vector\_i16 | 1000000 | 4.780 | 0.008330 |
| libsrt\_vector\_i32 | 1000000 | 6.704 | 0.038301 |
| cxx\_vector\_i32 | 1000000 | 6.812 | 0.009260 |
| libsrt\_vector\_i64 | 1000000 | 10.532 | 0.038458 |
| cxx\_vector\_i64 | 1000000 | 10.752 | 0.009944 |
| libsrt\_vector\_gen | 1000000 | 35.740 | 0.044044 |
| cxx\_vector\_gen | 1000000 | 34.368 | 0.019381 |

* Insert or process 1000000 elements, delete all elements one by one, cleanup

| Test | Insert count | Memory (MiB) | Execution time (s) |
|:---:|:---:|:---:|:---:|
| libsrt\_map\_ii32 | 1000000 | 18.496 | 0.719004 |
| cxx\_map\_ii32 | 1000000 | 49.476 | 0.345022 |
| cxx\_umap\_ii32 | 1000000 | 42.100 | 0.101379 |
| libsrt\_map\_ii64 | 1000000 | 27.520 | 0.679127 |
| cxx\_map\_ii64 | 1000000 | 65.248 | 0.385790 |
| cxx\_umap\_ii64 | 1000000 | 41.988 | 0.105119 |
| libsrt\_map\_s16 | 1000000 | 58.716 | 2.202747 |
| cxx\_map\_s16 | 1000000 | 185 | 1.151901 |
| cxx\_umap\_s16 | 1000000 | 178 | 1.156012 |
| libsrt\_map\_s64 | 1000000 | 209 | 2.806637 |
| cxx\_map\_s64 | 1000000 | 262 | 1.400506 |
| cxx\_umap\_s64 | 1000000 | 254 | 1.342591 |
| libsrt\_set\_i32 | 1000000 | 15.520 | 0.638711 |
| cxx\_set\_i32 | 1000000 | 49.476 | 0.336271 |
| libsrt\_set\_i64 | 1000000 | 19.236 | 0.732617 |
| cxx\_set\_i64 | 1000000 | 49.408 | 0.351317 |
| libsrt\_set\_s16 | 1000000 | 35.024 | 2.135677 |
| cxx\_set\_s16 | 1000000 | 109 | 0.968925 |
| libsrt\_set\_s64 | 1000000 | 109 | 2.617974 |
| cxx\_set\_s64 | 1000000 | 155 | 1.217954 |
| libsrt\_vector\_i8 | 1000000 | 3.764 | 0.040468 |
| cxx\_vector\_i8 | 1000000 | 3.708 | 0.001018 |
| libsrt\_vector\_i16 | 1000000 | 4.660 | 0.013788 |
| cxx\_vector\_i16 | 1000000 | 4.780 | 0.002042 |
| libsrt\_vector\_i32 | 1000000 | 6.704 | 0.013791 |
| cxx\_vector\_i32 | 1000000 | 6.812 | 0.002859 |
| libsrt\_vector\_i64 | 1000000 | 10.532 | 0.013826 |
| cxx\_vector\_i64 | 1000000 | 10.752 | 0.003618 |
| libsrt\_vector\_gen | 1000000 | 35.740 | 0.024923 |
| cxx\_vector\_gen | 1000000 | 34.368 | 0.013153 |

* Insert or process 1000000 elements, sort 10 times, cleanup

| Test | Insert count | Memory (MiB) | Execution time (s) |
|:---:|:---:|:---:|:---:|
| libsrt\_vector\_i8 | 1000000 | - | 0.027646 |
| cxx\_vector\_i8 | 1000000 | - | 0.137919 |
| libsrt\_vector\_u8 | 1000000 | - | 0.026851 |
| cxx\_vector\_u8 | 1000000 | - | 0.150012 |
| libsrt\_vector\_i16 | 1000000 | - | 0.145145 |
| cxx\_vector\_i16 | 1000000 | - | 0.145665 |
| libsrt\_vector\_u16 | 1000000 | - | 0.132487 |
| cxx\_vector\_u16 | 1000000 | - | 0.171347 |
| libsrt\_vector\_i32 | 1000000 | - | 0.183137 |
| cxx\_vector\_i32 | 1000000 | - | 0.150832 |
| libsrt\_vector\_u32 | 1000000 | - | 0.168793 |
| cxx\_vector\_u32 | 1000000 | - | 0.151224 |
| libsrt\_vector\_i64 | 1000000 | - | 0.189893 |
| cxx\_vector\_i64 | 1000000 | - | 0.161692 |
| libsrt\_vector\_u64 | 1000000 | - | 0.190136 |
| cxx\_vector\_u64 | 1000000 | - | 0.161340 |
| libsrt\_vector\_gen | 1000000 | - | 1.265519 |
| cxx\_vector\_gen | 1000000 | - | 2.052374 |

* Insert or process 1000000 elements (reverse order), sort 10 times, cleanup

| Test | Insert count | Memory (MiB) | Execution time (s) |
|:---:|:---:|:---:|:---:|
| libsrt\_vector\_i8 | 1000000 | - | 0.027181 |
| cxx\_vector\_i8 | 1000000 | - | 0.138454 |
| libsrt\_vector\_u8 | 1000000 | - | 0.027344 |
| cxx\_vector\_u8 | 1000000 | - | 0.149571 |
| libsrt\_vector\_i16 | 1000000 | - | 0.146363 |
| cxx\_vector\_i16 | 1000000 | - | 0.144867 |
| libsrt\_vector\_u16 | 1000000 | - | 0.132561 |
| cxx\_vector\_u16 | 1000000 | - | 0.173561 |
| libsrt\_vector\_i32 | 1000000 | - | 0.182960 |
| cxx\_vector\_i32 | 1000000 | - | 0.148663 |
| libsrt\_vector\_u32 | 1000000 | - | 0.169118 |
| cxx\_vector\_u32 | 1000000 | - | 0.149012 |
| libsrt\_vector\_i64 | 1000000 | - | 0.188534 |
| cxx\_vector\_i64 | 1000000 | - | 0.160186 |
| libsrt\_vector\_u64 | 1000000 | - | 0.190392 |
| cxx\_vector\_u64 | 1000000 | - | 0.159374 |
| libsrt\_vector\_gen | 1000000 | - | 1.263054 |
| cxx\_vector\_gen | 1000000 | - | 2.050574 |

* Insert or process 100 elements, sort 10000 times, cleanup

| Test | Insert count | Memory (MiB) | Execution time (s) |
|:---:|:---:|:---:|:---:|
| libsrt\_vector\_i8 | 100 | - | 0.005072 |
| cxx\_vector\_i8 | 100 | - | 0.005851 |
| libsrt\_vector\_u8 | 100 | - | 0.003463 |
| cxx\_vector\_u8 | 100 | - | 0.006403 |
| libsrt\_vector\_i16 | 100 | - | 0.009031 |
| cxx\_vector\_i16 | 100 | - | 0.005912 |
| libsrt\_vector\_u16 | 100 | - | 0.007740 |
| cxx\_vector\_u16 | 100 | - | 0.006440 |
| libsrt\_vector\_i32 | 100 | - | 0.008450 |
| cxx\_vector\_i32 | 100 | - | 0.006515 |
| libsrt\_vector\_u32 | 100 | - | 0.007627 |
| cxx\_vector\_u32 | 100 | - | 0.006043 |
| libsrt\_vector\_i64 | 100 | - | 0.008802 |
| cxx\_vector\_i64 | 100 | - | 0.006336 |
| libsrt\_vector\_u64 | 100 | - | 0.009422 |
| cxx\_vector\_u64 | 100 | - | 0.006278 |
| libsrt\_vector\_gen | 100 | - | 0.041418 |
| cxx\_vector\_gen | 100 | - | 0.048459 |

32-bit g++ 5.2.1 (x86\_32, Linux Ubuntu 15.10 x86\_64)
===

CPU: Intel Core i5-3330 @3GHz with 6MB L3 cache

* Insert or process 1000000 elements, cleanup

| Test | Insert count | Memory (MiB) | Execution time (s) |
|:---:|:---:|:---:|:---:|
| libsrt\_map\_ii32 | 1000000 | 18.068 | 0.398305 |
| cxx\_map\_ii32 | 1000000 | 33.432 | 0.283433 |
| cxx\_umap\_ii32 | 1000000 | 22.104 | 0.081586 |
| libsrt\_map\_ii64 | 1000000 | 26.544 | 0.379772 |
| cxx\_map\_ii64 | 1000000 | 41.320 | 0.264434 |
| cxx\_umap\_ii64 | 1000000 | 29.852 | 0.083150 |
| libsrt\_map\_s16 | 1000000 | 57.804 | 1.354844 |
| cxx\_map\_s16 | 1000000 | 132 | 0.784115 |
| cxx\_umap\_s16 | 1000000 | 128 | 1.108064 |
| libsrt\_map\_s64 | 1000000 | 208 | 1.620590 |
| cxx\_map\_s64 | 1000000 | 208 | 1.057254 |
| cxx\_umap\_s64 | 1000000 | 204 | 0.971725 |
| libsrt\_set\_i32 | 1000000 | 15.232 | 0.467258 |
| cxx\_set\_i32 | 1000000 | 25.892 | 0.230672 |
| libsrt\_set\_i64 | 1000000 | 18.796 | 0.392070 |
| cxx\_set\_i64 | 1000000 | 33.596 | 0.306909 |
| libsrt\_set\_s16 | 1000000 | 35.352 | 1.189445 |
| cxx\_set\_s16 | 1000000 | 72.660 | 0.654441 |
| libsrt\_set\_s64 | 1000000 | 110 | 1.409478 |
| cxx\_set\_s64 | 1000000 | 116 | 0.851388 |
| libsrt\_vector\_i8 | 1000000 | 3.524 | 0.033179 |
| cxx\_vector\_i8 | 1000000 | 3.316 | 0.001354 |
| libsrt\_vector\_i16 | 1000000 | 4.300 | 0.012616 |
| cxx\_vector\_i16 | 1000000 | 4.376 | 0.002903 |
| libsrt\_vector\_i32 | 1000000 | 6.208 | 0.013093 |
| cxx\_vector\_i32 | 1000000 | 6.380 | 0.003807 |
| libsrt\_vector\_i64 | 1000000 | 10.400 | 0.014899 |
| cxx\_vector\_i64 | 1000000 | 10.100 | 0.005410 |
| libsrt\_vector\_gen | 1000000 | 35.668 | 0.024917 |
| cxx\_vector\_gen | 1000000 | 33.708 | 0.015757 |
| libsrt\_string\_search\_easymatch\_long\_1a | 1000000 | - | 0.168221 |
| c\_string\_search\_easymatch\_long\_1a | 1000000 | - | 1.672853 |
| cxx\_string\_search\_easymatch\_long\_1a | 1000000 | - | 0.467530 |
| libsrt\_string\_search\_easymatch\_long\_1b | 1000000 | - | 0.197989 |
| c\_string\_search\_easymatch\_long\_1b | 1000000 | - | 0.282950 |
| cxx\_string\_search\_easymatch\_long\_1b | 1000000 | - | 0.327193 |
| libsrt\_string\_search\_easymatch\_long\_2a | 1000000 | - | 0.036346 |
| c\_string\_search\_easymatch\_long\_2a | 1000000 | - | 0.215154 |
| cxx\_string\_search\_easymatch\_long\_2a | 1000000 | - | 0.367246 |
| libsrt\_string\_search\_easymatch\_long\_2b | 1000000 | - | 0.285213 |
| c\_string\_search\_easymatch\_long\_2b | 1000000 | - | 0.948526 |
| cxx\_string\_search\_easymatch\_long\_2b | 1000000 | - | 0.661149 |
| libsrt\_string\_search\_hardmatch\_long\_1a | 1000000 | - | 0.154662 |
| c\_string\_search\_hardmatch\_long\_1a | 1000000 | - | 0.244479 |
| cxx\_string\_search\_hardmatch\_long\_1a | 1000000 | - | 1.408104 |
| libsrt\_string\_search\_hardmatch\_long\_1b | 1000000 | - | 0.153656 |
| c\_string\_search\_hardmatch\_long\_1b | 1000000 | - | 0.195657 |
| cxx\_string\_search\_hardmatch\_long\_1b | 1000000 | - | 1.058127 |
| libsrt\_string\_search\_hardmatch\_short\_1a | 1000000 | - | 0.045163 |
| c\_string\_search\_hardmatch\_short\_1a | 1000000 | - | 0.108502 |
| cxx\_string\_search\_hardmatch\_short\_1a | 1000000 | - | 0.085093 |
| libsrt\_string\_search\_hardmatch\_short\_1b | 1000000 | - | 0.042866 |
| c\_string\_search\_hardmatch\_short\_1b | 1000000 | - | 0.067985 |
| cxx\_string\_search\_hardmatch\_short\_1b | 1000000 | - | 0.109399 |
| libsrt\_string\_search\_hardmatch\_long\_2 | 1000000 | - | 0.262035 |
| c\_string\_search\_hardmatch\_long\_2 | 1000000 | - | 0.447266 |
| cxx\_string\_search\_hardmatch\_long\_2 | 1000000 | - | 0.351104 |
| libsrt\_string\_search\_hardmatch\_long\_3 | 1000000 | - | 0.831276 |
| c\_string\_search\_hardmatch\_long\_3 | 1000000 | - | 4.586305 |
| cxx\_string\_search\_hardmatch\_long\_3 | 1000000 | - | 21.785857 |
| c\_string\_loweruppercase\_ascii | 1000000 | - | 0.851290 |
| cxx\_string\_loweruppercase\_ascii | 1000000 | - | 0.161103 |
| c\_string\_loweruppercase\_utf8 | 1000000 | - | 0.851371 |
| cxx\_string\_loweruppercase\_utf8 | 1000000 | - | 0.160595 |
| libsrt\_bitset | 1000000 | 1.456 | 0.004072 |
| cxx\_bitset | 1000000 | 1.496 | 0.001534 |
| libsrt\_bitset\_popcount100 | 1000000 | 1.548 | 0.007669 |
| cxx\_bitset\_popcount100 | 1000000 | 1.440 | 0.023218 |
| libsrt\_bitset\_popcount10000 | 1000000 | 1.548 | 0.006772 |
| cxx\_bitset\_popcount10000 | 1000000 | 1.440 | 1.238345 |
| libsrt\_string\_cat | 1000000 | - | 0.589623 |
| c\_string\_cat | 1000000 | - | 3.825385 |
| cxx\_string\_cat | 1000000 | - | 0.460933 |

* Insert 1000000 elements, read all elements 10 times, cleanup

| Test | Insert count | Memory (MiB) | Execution time (s) |
|:---:|:---:|:---:|:---:|
| libsrt\_map\_ii32 | 1000000 | 18.068 | 1.961523 |
| cxx\_map\_ii32 | 1000000 | 33.432 | 1.777760 |
| cxx\_umap\_ii32 | 1000000 | 22.104 | 0.160181 |
| libsrt\_map\_ii64 | 1000000 | 26.544 | 2.257002 |
| cxx\_map\_ii64 | 1000000 | 41.320 | 1.499069 |
| cxx\_umap\_ii64 | 1000000 | 29.852 | 0.184117 |
| libsrt\_map\_s16 | 1000000 | 57.804 | 8.962178 |
| cxx\_map\_s16 | 1000000 | 132 | 4.920313 |
| cxx\_umap\_s16 | 1000000 | 128 | 5.786537 |
| libsrt\_map\_s64 | 1000000 | 208 | 10.669136 |
| cxx\_map\_s64 | 1000000 | 208 | 6.848690 |
| cxx\_umap\_s64 | 1000000 | 204 | 6.531616 |
| libsrt\_set\_i32 | 1000000 | 15.232 | 1.933994 |
| cxx\_set\_i32 | 1000000 | 25.892 | 1.389893 |
| libsrt\_set\_i64 | 1000000 | 18.796 | 2.375171 |
| cxx\_set\_i64 | 1000000 | 33.596 | 1.835748 |
| libsrt\_set\_s16 | 1000000 | 35.352 | 7.931001 |
| cxx\_set\_s16 | 1000000 | 72.660 | 4.799833 |
| libsrt\_set\_s64 | 1000000 | 110 | 10.410957 |
| cxx\_set\_s64 | 1000000 | 116 | 6.423329 |
| libsrt\_vector\_i8 | 1000000 | 3.524 | 0.066086 |
| cxx\_vector\_i8 | 1000000 | 3.316 | 0.007628 |
| libsrt\_vector\_i16 | 1000000 | 4.300 | 0.046176 |
| cxx\_vector\_i16 | 1000000 | 4.376 | 0.009237 |
| libsrt\_vector\_i32 | 1000000 | 6.208 | 0.046815 |
| cxx\_vector\_i32 | 1000000 | 6.380 | 0.010056 |
| libsrt\_vector\_i64 | 1000000 | 10.400 | 0.049270 |
| cxx\_vector\_i64 | 1000000 | 10.100 | 0.011624 |
| libsrt\_vector\_gen | 1000000 | 35.668 | 0.048014 |
| cxx\_vector\_gen | 1000000 | 33.708 | 0.022217 |

* Insert or process 1000000 elements, delete all elements one by one, cleanup

| Test | Insert count | Memory (MiB) | Execution time (s) |
|:---:|:---:|:---:|:---:|
| libsrt\_map\_ii32 | 1000000 | 18.068 | 0.768011 |
| cxx\_map\_ii32 | 1000000 | 33.432 | 0.384258 |
| cxx\_umap\_ii32 | 1000000 | 22.104 | 0.089723 |
| libsrt\_map\_ii64 | 1000000 | 26.544 | 0.818978 |
| cxx\_map\_ii64 | 1000000 | 41.320 | 0.362104 |
| cxx\_umap\_ii64 | 1000000 | 29.852 | 0.093193 |
| libsrt\_map\_s16 | 1000000 | 57.804 | 2.856854 |
| cxx\_map\_s16 | 1000000 | 132 | 1.268331 |
| cxx\_umap\_s16 | 1000000 | 128 | 1.134999 |
| libsrt\_map\_s64 | 1000000 | 208 | 3.392194 |
| cxx\_map\_s64 | 1000000 | 208 | 1.721878 |
| cxx\_umap\_s64 | 1000000 | 204 | 1.311404 |
| libsrt\_set\_i32 | 1000000 | 15.232 | 0.691827 |
| cxx\_set\_i32 | 1000000 | 25.892 | 0.312196 |
| libsrt\_set\_i64 | 1000000 | 18.796 | 0.857881 |
| cxx\_set\_i64 | 1000000 | 33.596 | 0.425811 |
| libsrt\_set\_s16 | 1000000 | 35.352 | 2.585487 |
| cxx\_set\_s16 | 1000000 | 72.660 | 1.141357 |
| libsrt\_set\_s64 | 1000000 | 110 | 3.231993 |
| cxx\_set\_s64 | 1000000 | 116 | 1.505946 |
| libsrt\_vector\_i8 | 1000000 | 3.524 | 0.047799 |
| cxx\_vector\_i8 | 1000000 | 3.316 | 0.001347 |
| libsrt\_vector\_i16 | 1000000 | 4.300 | 0.017583 |
| cxx\_vector\_i16 | 1000000 | 4.376 | 0.002922 |
| libsrt\_vector\_i32 | 1000000 | 6.208 | 0.018088 |
| cxx\_vector\_i32 | 1000000 | 6.380 | 0.003701 |
| libsrt\_vector\_i64 | 1000000 | 10.400 | 0.020181 |
| cxx\_vector\_i64 | 1000000 | 10.100 | 0.005462 |
| libsrt\_vector\_gen | 1000000 | 35.668 | 0.027701 |
| cxx\_vector\_gen | 1000000 | 33.708 | 0.015934 |


64-bit g++ 4.9.2 (aarch64, Linux Debian 8 aarch64)
===

CPU: Allwinner A64 (ARM Cortex A53) @1.2GHz with 512KB L2 cache

Note: libsrt RAM usage similar to the x86\_64 case, however, the STL (C++) memory usage can be different, because of different compiler version

* Insert or process 1000000 elements, cleanup

| Test | Insert count | Memory (MiB) | Execution time (s) |
|:---:|:---:|:---:|:---:|
| libsrt\_map\_ii32 | 1000000 | - | 3.801208 |
| cxx\_map\_ii32 | 1000000 | - | 1.358137 |
| cxx\_umap\_ii32 | 1000000 | - | 0.534476 |
| libsrt\_map\_ii64 | 1000000 | - | 2.698025 |
| cxx\_map\_ii64 | 1000000 | - | 1.676018 |
| cxx\_umap\_ii64 | 1000000 | - | 0.550315 |
| libsrt\_map\_s16 | 1000000 | - | 9.285355 |
| cxx\_map\_s16 | 1000000 | - | 5.669090 |
| cxx\_umap\_s16 | 1000000 | - | 6.299354 |
| libsrt\_map\_s64 | 1000000 | - | 15.642442 |
| cxx\_map\_s64 | 1000000 | - | 6.947106 |
| cxx\_umap\_s64 | 1000000 | - | 7.728419 |
| libsrt\_set\_i32 | 1000000 | - | 2.543004 |
| cxx\_set\_i32 | 1000000 | - | 1.349157 |
| libsrt\_set\_i64 | 1000000 | - | 4.278662 |
| cxx\_set\_i64 | 1000000 | - | 1.333109 |
| libsrt\_set\_s16 | 1000000 | - | 9.365514 |
| cxx\_set\_s16 | 1000000 | - | 4.665136 |
| libsrt\_set\_s64 | 1000000 | - | 11.837463 |
| cxx\_set\_s64 | 1000000 | - | 5.725180 |
| libsrt\_vector\_i8 | 1000000 | - | 0.569887 |
| cxx\_vector\_i8 | 1000000 | - | 0.010571 |
| libsrt\_vector\_i16 | 1000000 | - | 0.069697 |
| cxx\_vector\_i16 | 1000000 | - | 0.014801 |
| libsrt\_vector\_i32 | 1000000 | - | 0.069779 |
| cxx\_vector\_i32 | 1000000 | - | 0.018530 |
| libsrt\_vector\_i64 | 1000000 | - | 0.069945 |
| cxx\_vector\_i64 | 1000000 | - | 0.027682 |
| libsrt\_vector\_gen | 1000000 | - | 0.192064 |
| cxx\_vector\_gen | 1000000 | - | 0.105553 |
| libsrt\_string\_search\_easymatch\_long\_1a | 1000000 | - | 1.488772 |
| c\_string\_search\_easymatch\_long\_1a | 1000000 | - | 6.922350 |
| cxx\_string\_search\_easymatch\_long\_1a | 1000000 | - | 2.964655 |
| libsrt\_string\_search\_easymatch\_long\_1b | 1000000 | - | 1.721810 |
| c\_string\_search\_easymatch\_long\_1b | 1000000 | - | 2.471100 |
| cxx\_string\_search\_easymatch\_long\_1b | 1000000 | - | 1.900162 |
| libsrt\_string\_search\_easymatch\_long\_2a | 1000000 | - | 0.792486 |
| c\_string\_search\_easymatch\_long\_2a | 1000000 | - | 0.914612 |
| cxx\_string\_search\_easymatch\_long\_2a | 1000000 | - | 2.434237 |
| libsrt\_string\_search\_easymatch\_long\_2b | 1000000 | - | 2.566107 |
| c\_string\_search\_easymatch\_long\_2b | 1000000 | - | 5.674909 |
| cxx\_string\_search\_easymatch\_long\_2b | 1000000 | - | 5.231105 |
| libsrt\_string\_search\_hardmatch\_long\_1a | 1000000 | - | 1.367257 |
| c\_string\_search\_hardmatch\_long\_1a | 1000000 | - | 1.921811 |
| cxx\_string\_search\_hardmatch\_long\_1a | 1000000 | - | 11.835561 |
| libsrt\_string\_search\_hardmatch\_long\_1b | 1000000 | - | 1.320063 |
| c\_string\_search\_hardmatch\_long\_1b | 1000000 | - | 1.600803 |
| cxx\_string\_search\_hardmatch\_long\_1b | 1000000 | - | 6.195738 |
| libsrt\_string\_search\_hardmatch\_short\_1a | 1000000 | - | 0.366944 |
| c\_string\_search\_hardmatch\_short\_1a | 1000000 | - | 0.623199 |
| cxx\_string\_search\_hardmatch\_short\_1a | 1000000 | - | 0.735919 |
| libsrt\_string\_search\_hardmatch\_short\_1b | 1000000 | - | 0.292726 |
| c\_string\_search\_hardmatch\_short\_1b | 1000000 | - | 0.366521 |
| cxx\_string\_search\_hardmatch\_short\_1b | 1000000 | - | 0.624152 |
| libsrt\_string\_search\_hardmatch\_long\_2 | 1000000 | - | 1.522058 |
| c\_string\_search\_hardmatch\_long\_2 | 1000000 | - | 3.175667 |
| cxx\_string\_search\_hardmatch\_long\_2 | 1000000 | - | 2.238520 |
| libsrt\_string\_search\_hardmatch\_long\_3 | 1000000 | - | 6.976331 |
| c\_string\_search\_hardmatch\_long\_3 | 1000000 | - | 27.925946 |
| cxx\_string\_search\_hardmatch\_long\_3 | 1000000 | - | 986.937059 |
| c\_string\_loweruppercase\_ascii | 1000000 | - | 1.226957 |
| cxx\_string\_loweruppercase\_ascii | 1000000 | - | 1.238296 |
| c\_string\_loweruppercase\_utf8 | 1000000 | - | 1.227270 |
| cxx\_string\_loweruppercase\_utf8 | 1000000 | - | 1.239529 |
| libsrt\_bitset | 1000000 | - | 0.029115 |
| cxx\_bitset | 1000000 | - | 0.016703 |
| libsrt\_bitset\_popcount100 | 1000000 | - | 0.070534 |
| cxx\_bitset\_popcount100 | 1000000 | - | 0.118657 |
| libsrt\_bitset\_popcount10000 | 1000000 | - | 0.069910 |
| cxx\_bitset\_popcount10000 | 1000000 | - | 4.281931 |
| libsrt\_string\_cat | 1000000 | - | 3.765053 |
| c\_string\_cat | 1000000 | - | 24.344990 |
| cxx\_string\_cat | 1000000 | - | 2.569641 |

* Insert 1000000 elements, read all elements 10 times, cleanup

| Test | Insert count | Memory (MiB) | Execution time (s) |
|:---:|:---:|:---:|:---:|
| libsrt\_map\_ii32 | 1000000 | - | 10.129269 |
| cxx\_map\_ii32 | 1000000 | - | 6.485455 |
| cxx\_umap\_ii32 | 1000000 | - | 0.906621 |
| libsrt\_map\_ii64 | 1000000 | - | 10.395867 |
| cxx\_map\_ii64 | 1000000 | - | 8.176145 |
| cxx\_umap\_ii64 | 1000000 | - | 0.903205 |
| libsrt\_map\_s16 | 1000000 | - | 54.965830 |
| cxx\_map\_s16 | 1000000 | - | 34.946391 |
| cxx\_umap\_s16 | 1000000 | - | 32.779182 |
| libsrt\_map\_s64 | 1000000 | - | 102.364240 |
| cxx\_map\_s64 | 1000000 | - | 46.153162 |
| cxx\_umap\_s64 | 1000000 | - | 40.164621 |
| libsrt\_set\_i32 | 1000000 | - | 9.453710 |
| cxx\_set\_i32 | 1000000 | - | 6.551233 |
| libsrt\_set\_i64 | 1000000 | - | 10.501685 |
| cxx\_set\_i64 | 1000000 | - | 6.286232 |
| libsrt\_set\_s16 | 1000000 | - | 56.196831 |
| cxx\_set\_s16 | 1000000 | - | 34.408315 |
| libsrt\_set\_s64 | 1000000 | - | 96.076725 |
| cxx\_set\_s64 | 1000000 | - | 43.780441 |
| libsrt\_vector\_i8 | 1000000 | - | 0.739497 |
| cxx\_vector\_i8 | 1000000 | - | 0.036437 |
| libsrt\_vector\_i16 | 1000000 | - | 0.245701 |
| cxx\_vector\_i16 | 1000000 | - | 0.041074 |
| libsrt\_vector\_i32 | 1000000 | - | 0.247528 |
| cxx\_vector\_i32 | 1000000 | - | 0.045071 |
| libsrt\_vector\_i64 | 1000000 | - | 0.252332 |
| cxx\_vector\_i64 | 1000000 | - | 0.053330 |
| libsrt\_vector\_gen | 1000000 | - | 0.305215 |
| cxx\_vector\_gen | 1000000 | - | 0.131958 |

* Insert or process 1000000 elements, delete all elements one by one, cleanup

| Test | Insert count | Memory (MiB) | Execution time (s) |
|:---:|:---:|:---:|:---:|
| libsrt\_map\_ii32 | 1000000 | - | 5.127531 |
| cxx\_map\_ii32 | 1000000 | - | 1.855783 |
| cxx\_umap\_ii32 | 1000000 | - | 0.585743 |
| libsrt\_map\_ii64 | 1000000 | - | 4.963188 |
| cxx\_map\_ii64 | 1000000 | - | 2.281410 |
| cxx\_umap\_ii64 | 1000000 | - | 0.603728 |
| libsrt\_map\_s16 | 1000000 | - | 18.671822 |
| cxx\_map\_s16 | 1000000 | - | 8.524266 |
| cxx\_umap\_s16 | 1000000 | - | 6.976797 |
| libsrt\_map\_s64 | 1000000 | - | 31.822136 |
| cxx\_map\_s64 | 1000000 | - | 11.223638 |
| cxx\_umap\_s64 | 1000000 | - | 9.014059 |
| libsrt\_set\_i32 | 1000000 | - | 4.724335 |
| cxx\_set\_i32 | 1000000 | - | 1.888834 |
| libsrt\_set\_i64 | 1000000 | - | 5.376476 |
| cxx\_set\_i64 | 1000000 | - | 1.875787 |
| libsrt\_set\_s16 | 1000000 | - | 18.487051 |
| cxx\_set\_s16 | 1000000 | - | 7.950109 |
| libsrt\_set\_s64 | 1000000 | - | 28.352386 |
| cxx\_set\_s64 | 1000000 | - | 10.176614 |
| libsrt\_vector\_i8 | 1000000 | - | 0.585575 |
| cxx\_vector\_i8 | 1000000 | - | 0.010299 |
| libsrt\_vector\_i16 | 1000000 | - | 0.089791 |
| cxx\_vector\_i16 | 1000000 | - | 0.014778 |
| libsrt\_vector\_i32 | 1000000 | - | 0.090039 |
| cxx\_vector\_i32 | 1000000 | - | 0.018494 |
| libsrt\_vector\_i64 | 1000000 | - | 0.090338 |
| cxx\_vector\_i64 | 1000000 | - | 0.027295 |
| libsrt\_vector\_gen | 1000000 | - | 0.204283 |
| cxx\_vector\_gen | 1000000 | - | 0.105846 |


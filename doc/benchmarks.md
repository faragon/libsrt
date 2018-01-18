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


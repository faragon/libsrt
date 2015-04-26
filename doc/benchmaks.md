Common
===

* umap: g++ unordered_map
* map: g++ map
* smap: gcc libsrt smap
* gcc/g++ 4.8.2-19ubuntu1 (x86_64), Ubuntu 14.04, Intel i5-3330 (3.00GHz 6144KB cache)


String benchmarks
===

To do: copy, search, case, etc.

Vector benchmarks
===

Tree benchmarks
===

Map benchmarks
===

Integer-key map insert
---

```cpp
#ifdef T_MAP
        std::map<unsigned,unsigned> m;
#else
        std::unordered_map<unsigned,unsigned> m;
#endif
        for (size_t i = 0; i < count; i++)
                m[i] = 1;
```
```c
        sm_t *m = sm_alloc(SM_U32U32, 1);
        for (size_t i = 0; i < count; i++)
                sm_uu32_insert(&m, i, 1);
        sm_free(&m);
```

* umap: g++ unordered_map
* map: g++ map
* smap: gcc libsrt smap (starting with just one element allocated)
* gcc/g++ 4.8.2-19ubuntu1, Ubuntu 14.04, Intel i5-3330 (3.00GHz 6144KB cache)

| Test | Insert count | Memory (MB) | Execution time (s) |
| ------------------- |:----:|:----:|:-----:|
| umap   | 3*10^5 | 14 | 0.034 |
| map    | 3*10^5 | 14.5 | 0.080 |
| smap   | 3*10^5 | 5 | 0.105 |
| umap   | 10^6 | 40.5 | 0.089 |
| map    | 10^6 | 48   | 0.342 |
| smap   | 10^6 | 16   | 0.383 |
| umap   | 10^7 | 463.9 | 0.948 |
| map    | 10^7 | 480 | 4.771 |
| smap   | 10^7 | 161.5 | 4.786 |
| umap   | 10^8 | 4402 | 9.031 |
| map    | 10^8 | 4831 | 63.444 |
| smap   | 10^8 | 1610 | 59.804 |

In this case, libsrt smap wins in the memory usage to both GNU`s std::map (RB tree) and std::unordered_map (hash map). In speed terms, libsrt smap and std::map are similar (same algorithm), while std::unordered_map wins because using both hash tables and being the hashing of 32-bit integer very simple.

Note that "average" insertion time complexity for std::unordered_map is O(1), while std::map and libsrt smap have O(log(n)). At 10^6 elements being 5x faster, and being just 6.6x faster at 10^8 elements, it shows that the allocator is hurting the hash table performance. Currently libsrt smap is not as optimized as it could be (more optimizations will be added after being sure there are no implementation errors left), the target will be being competitive with hash tables up to 10^7 elements for the case of int-int maps (e.g. 2-3x speed difference, instead of 5x).

Integer-key map insert and 10x read
---

```cpp
#ifdef T_MAP
        std::map<unsigned,unsigned> m;
#else
        std::unordered_map<unsigned,unsigned> m;
#endif
        for (size_t i = 0; i < count; i++)
                m[i] = 1;
	for (size_t h = 0; h < 10; h++)
		for (size_t i = 0; i < count; i++)
			if (m.count(i) == 0) {
				fprintf(stderr, "map error\n");
				exit(1);
			}
```
```c
        sm_t *m = sm_alloc(SM_U32U32, 1);
        for (size_t i = 0; i < count; i++)
                sm_uu32_insert(&m, i, 1);
	for (size_t h = 0; h < 10; h++)
		for (size_t i = 0; i < count; i++)
			if (sm_u_count(m, i) == 0) {
				fprintf(stderr, "map error\n");
				exit(1);
			}
	sm_free(&m);
```

| Test | Insert count | Read count | Memory (MB) | Execution time (s) |
| ------------------- |:----:|:----:|:----:|:-----:|
| umap   | 10^6 | 10^7 | 40.5 | 0.284 |
| map    | 10^6 | 10^7 | 48   | 1.965 |
| smap   | 10^6 | 10^7 | 16   | 2.133 |
| umap   | 10^7 | 10^8 | 463.9 | 2.869 |
| map    | 10^7 | 10^8 | 480   | 29.026 |
| smap   | 10^7 | 10^8 | 161.5 | 29.386 |
| umap   | 10^8 | 10^9 | 4402 | 28.060 |
| map    | 10^8 | 10^9 | 4831 | 412.542 |
| smap   | 10^8 | 10^9 | 1610 | 433.407 |

Because of access not having the penalty of memory allocation, the std::unordered_map is 3x faster on read than on insertion. After optimizations, libsrt smap will target to reduce current 10x to 5x time vs the hash map case for 10^7 elements int-int maps.

String-key map insert
---


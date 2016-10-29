Common
===

* umap: g++ unordered\_map
* map: g++ map
* smap: gcc libsrt smap
* gcc/g++ 4.8.2-19ubuntu1 (x86\_64) (optimization flags: -O2), Ubuntu 14.04, Intel i5-3330 (3.00GHz 6144KB cache)

Detailed benchmarks will be added once the API gets freezed. Currently only integer map benchmarks are available. If you have tests done by yourself, and willing to share, you're welcome.

String benchmarks (ss\_t)
===

Introduction
---

To do: copy, search, case, etc.

Vector benchmarks (sv\_t)
===

Introduction
---

Tree benchmarks (st\_t)
===

Introduction
---

Map benchmarks (sm\_t)
===

Introduction
---

libsrt map implementation use red-black trees. It is about 10 times slower than hash tables (e.g. std::unsorted\_map) for maps with up to 10^8 elements doing search on simplest data types. E.g. just doing search, with same CPU used for the tests, you could query about 6 million of nodes per second (int-int type) while with the hash map, more than 60 million per second (in both cases with one CPU). For more complex data structures in the map, or including interface operations like serialization/deserialization, the total per-query time gets closer between the tree-based implementation and the hash-map implementation (e.g. serving 1 million key-value queries per second on one CPU is a huge achievement, so having 6 or 60 million QPS throughput is not going to make a world of difference). Regarding memory usage, libsrt map uses one third of libstdc++ map (RB tree) and unordered\_map (hash tables) for the case of int-int, i.e. with same memory, it can hold 3 times more data in that case.

There is room for some optimization on the libsrt map (sm\_t), but bigger gains come from using distributed maps (see sdm\_t type, libstrt map implementation that manages N maps with same interface as the simple map), so micro-optimization is not a priority (e.g. some unrolling and explicit prefetch could be added for helping non-OooE CPUs -both optimizations tried on OooE CPUs, with no visible gain, even inlining the compare function instead of using a function pointer; more tests will be done with a simpler CPU, e.g. board with ARM v6 ISA CPU-).

Integer-key map insert
---

```cpp
#ifdef T_MAP
        std::map<unsigned,unsigned> m;
#else
        std::unordered\_map<unsigned,unsigned> m;
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
* umap: g++ unordered\_map
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

In this case, libsrt smap wins in the memory usage to both GNU`s std::map (RB tree) and std::unordered\_map (hash map). In speed terms, libsrt smap and std::map are similar (same algorithm), while std::unordered\_map wins because using both hash tables and being the hashing of 32-bit integer very simple.


Integer-key map insert and 10x read
---

```cpp
#ifdef T\_MAP
        std::map<unsigned,unsigned> m;
#else
        std::unordered\_map<unsigned,unsigned> m;
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
        sm_t *m = sm\_alloc(SM\_U32U32, 1);
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

Because of access not having the penalty of memory allocation, the std::unordered\_map is 3x faster on read than on insertion.

String-key map insert
---


Distributed map benchmarks (sdm\_t)
===

Introduction
---


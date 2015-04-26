String benchmarks
===

Copy
---
 
Search
---

Case conversion
---

Vector benchmarks
===

Tree benchmarks
===

Map benchmarks
===

Integer-key maps
---

```cpp
        std::unordered_map<unsigned,unsigned> m;
        for (size_t i = 0; i < count; i++)
                m[i] = 1;
```
```cpp
        std::map<unsigned,unsigned> m;
        for (size_t i = 0; i < count; i++)
                m[i] = 1;
```
```c
        sm_t *m = sm_alloc(SM_U32U32, count);
        for (size_t i = 0; i < count; i++)
                sm_uu32_insert(&m, i, 1);
        sm_free(&m);
```

umap: g++ unordered_map
map: g++ map
smap: gcc libsrt smap
gcc/g++ 4.8.2-19ubuntu1, Ubuntu 14.04, Intel i5-3330 (3.00GHz 6144KB cache)

| Test | Element count | Memory (MB) | Execution time (s) |
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

In this case, libsrt smap wins in the memory usage to both GNU`s std::map (RB tree) and std::unordered_map (hash map). In speed terms, libsrt smap and std::map are similar (same algorithm), while std::unordered_map wins because using both hash tables and being the hashing of 32-bit integer very simple. Currently libsrt smap is not as optimized as it could be (more optimizations will be added after being sure there are no implementation errors left), the target will be being competitive with hash tables for up to 10^7 elements for the case of int-int maps.

String-key maps
---


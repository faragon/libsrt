Benchmarks overview
===

* 'bench' executable is built using 'make'
* Memory usage retrieved manually with htop
* Benchmark time precission limited to clock\_gettime() resolution on Linux
* Some functions are not yet optimized (e.g. vector)

64-bit g++ 5.2.1 (x86\_64, Linux Ubuntu 15.10 x86\_64)
===

* Insert or process 1000000 elements, cleanup

| Test | Insert count | Memory (MB) | Execution time (s) |
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
| libsrt\_set\_i64 | 1000000 | 19.236 | 0.359219 |
| cxx\_set\_i64 | 1000000 | 49.408 | 0.249820 |
| libsrt\_set\_s16 | 1000000 | 35.024 | 1.001737 |
| cxx\_set\_s16 | 1000000 | 109 | 0.555505 |
| libsrt\_set\_s64 | 1000000 | 109 | 1.226219 |
| cxx\_set\_s64 | 1000000 | 155 | 0.707712 |
| libsrt\_vector\_i8 | 1000000 | 6.748 | 0.036791 |
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
| libsrt\_bitset\_popcount100 | 1000000 | 1.760 | 0.003707 |
| cxx\_bitset\_popcount100 | 1000000 | 1.896 | 0.001753 |
| libsrt\_string\_cat | 1000000 | - | 0.659209 |
| c\_string\_cat | 1000000 | - | 3.496450 |
| cxx\_string\_cat | 1000000 | - | 0.376064 |

* Insert 1000000 elements, read all elements 10 times, cleanup

| Test | Insert count | Memory (MB) | Execution time (s) |
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
| libsrt\_set\_i64 | 1000000 | 19.236 | 1.420809 |
| cxx\_set\_i64 | 1000000 | 49.408 | 1.589099 |
| libsrt\_set\_s16 | 1000000 | 35.024 | 6.685535 |
| cxx\_set\_s16 | 1000000 | 109 | 4.177632 |
| libsrt\_set\_s64 | 1000000 | 109 | 8.631935 |
| cxx\_set\_s64 | 1000000 | 155 | 5.619492 |
| libsrt\_vector\_i8 | 1000000 | 6.748 | 0.065215 |
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

| Test | Insert count | Memory (MB) | Execution time (s) |
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
| libsrt\_set\_i64 | 1000000 | 19.236 | 0.649075 |
| cxx\_set\_i64 | 1000000 | 49.408 | 0.351317 |
| libsrt\_set\_s16 | 1000000 | 35.024 | 2.135677 |
| cxx\_set\_s16 | 1000000 | 109 | 0.968925 |
| libsrt\_set\_s64 | 1000000 | 109 | 2.617974 |
| cxx\_set\_s64 | 1000000 | 155 | 1.217954 |
| libsrt\_vector\_i8 | 1000000 | 6.748 | 0.040998 |
| cxx\_vector\_i8 | 1000000 | 3.708 | 0.001018 |
| libsrt\_vector\_i16 | 1000000 | 4.660 | 0.013788 |
| cxx\_vector\_i16 | 1000000 | 4.780 | 0.002042 |
| libsrt\_vector\_i32 | 1000000 | 6.704 | 0.013791 |
| cxx\_vector\_i32 | 1000000 | 6.812 | 0.002859 |
| libsrt\_vector\_i64 | 1000000 | 10.532 | 0.013826 |
| cxx\_vector\_i64 | 1000000 | 10.752 | 0.003618 |
| libsrt\_vector\_gen | 1000000 | 35.740 | 0.024923 |
| cxx\_vector\_gen | 1000000 | 34.368 | 0.013153 |

32-bit g++ 5.2.1 (x86\_32, Linux Ubuntu 15.10 x86\_64)
===


Benchmarks overview
===

* 'bench' executable is built using 'make'
* Memory usage retrieved manually with htop (that's the reason for using MiB instead of MB as unit)
* Memory usage does not reflect real per-element allocation, because of resizing heuristics, both for libsrt (C/C++) and STL (C++). E.g. a 10^6 byte vector shows using 3.5 MiB (~3.6 bytes per element) while a 10^8 would take 100 MiB (closer to the expected 1 byte per element).
* Benchmark time precision limited to clock\_gettime() resolution on Linux
* Some functions are not yet optimized (e.g. vector)

64-bit g++ 5.2.1 (x86\_64, Linux Ubuntu 15.10 x86\_64)
===

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
| libsrt\_set\_i64 | 1000000 | 19.236 | 0.359219 |
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
| libsrt\_bitset\_popcount100 | 1000000 | 1.760 | 0.003707 |
| cxx\_bitset\_popcount100 | 1000000 | 1.896 | 0.001753 |
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
| libsrt\_set\_i64 | 1000000 | 19.236 | 1.420809 |
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
| libsrt\_set\_i64 | 1000000 | 19.236 | 0.649075 |
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


32-bit g++ 5.2.1 (x86\_32, Linux Ubuntu 15.10 x86\_64)
===

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
| libsrt\_set\_i64 | 1000000 | 18.796 | 0.392326 |
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
| libsrt\_bitset\_popcount100 | 1000000 | 1.548 | 0.004130 |
| cxx\_bitset\_popcount100 | 1000000 | 1.440 | 0.001512 |
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
| libsrt\_set\_i64 | 1000000 | 18.796 | 1.781933 |
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
| libsrt\_set\_i64 | 1000000 | 18.796 | 0.752014 |
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



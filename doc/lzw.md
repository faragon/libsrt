# Time-optimized LZW (with RLE opcode mixing)

There are already fast LZ77 implementations (LZF, LZ4). This document is a design for a fast LZW implementation with built-in RLE opcode mixing (inject RLE opcodes into the LZW stream).

Copyright (c) 2015 F. Aragon. All rights reserved.

# Algorithm

## Introduction

LZW data compression algorithm (Welch, 1984) is derivated from LZW78 (Lempel and Ziv, 1978). LZW implementation equals to a LZW with first N codes already entered (e.g. N = 256, for coding general purpose per-byte encoding).

An hybrid LZW + RLE is implmented, being RLE opcodes injected/mixed into the LZW stream for covering 1, 2 and 4-byte patterns (that improves compression speed on sparse data). Current implementation encodes at 75-300 MB/s and decodes at 100-500 MB/s on Intel Core i5 @3GHz (one thread). On "flat" areas (same byte repeated many times), when RLE opcodes can be injected in the LZW stream, speed is much faster: up to 6 GB/s compression and 20 GB/s decompression (i5 @3GHz with dual channel DDR3 memory). For ARM 11 @700MHz (e.g. Raspberry Pi 1), encoding speed is 2-12 MB/s for "normal" data and up to 130 MB/s for 1/2/4 byte repeated sequences (when most LZW opcodes are RLE), being decoding speed 2-34 MB/s for "normal" data and up to 400 MB/s for the case of data with most LZW codes being RLE.

Uncompressible data is kept below input data length + 5% (this figure could change, still being tuned).

For LZW and RLE algorithm overview you can check following links:

- https://en.wikipedia.org/wiki/Lempel%E2%80%93Ziv%E2%80%93Welch
- http://www2.scssoft.com/~petr/gfx/lzw.html
- https://es.wikipedia.org/wiki/Run-length\_encoding

## Data structures

Most LZW implementations, for a given tree node, use a linked list plus some LUTs for speeding-up the search on dense nodes. When a node has few entries, e.g. being able to fit into one data cache line, there is almost no performance impact for sequential search (e.g. 4 to 6 element linked list).

### Memory usage reduction optimization: encoding

* Use 16-bit indexes instead of pointers (half the memory in 32-bit mode, one quarter in 64-bit mode).
* Avoid structure usage: use separated arrays, so no memory is wasted because of data alignment.
* Don't use linked lists: one node will have one child or a LUT as child. That will work very well for not "very random" data, while exausting fast LUT tables on noisy/random input. 
* Full node (with LUT reference): 5 + 256 bytes  2 bytes for node code (the element being stored as "compressed" data), 2 bytes multiplexed for LUT/code (if > 0, LUT for 256 child nodes, 0: empty, < 0: -(child node)), 1 byte for child node byte.
* Minimum node size (node with one child): 5 bytes (amortized alignment, so really 5 bytes, not more). This is a subset of the above with lut reference lower than 0 (using the negative number as code instead of LUT reference)

With the above, quite good 12/13-bit LZW compression could be run with 200-300KB of stack memory (not using the heap allocator). Memory requirement could be reduced up to 32-64KB limiting dictionay to e.g. 11 bits and reducing the number of available LUTs. In terms of speed, the 11 bit would be as fast as the 13-bit, as soon as the CPU has enough data cache (O(n) time complexity).

Every byte is passed trough the tree. The tree walk is stopped when reaching a leaf. The output code is the leaf code, and a new node with the new code is added as child of that leaf (if not running out of codes, e.g. for a 12-bit LZW, we'll have 2^12 codes/nodes -4096-).

In addition to the first 256 predefined codes, there is the "clear code" and "end of information code". Other codes could be added, e.g. "run length code", "uncompressed data code", or whatever other custom thing.

### Algorithm optimization: encoding

* Use first 256 node entries as a LUT itself for the root node
* Interleave one-child and N-child search in one loop
* Think about adding Rabin-Karp 8 to 16 byte rolling hash plus some additional hash tables (not yet decided). This could slowdown the loop for some cases, not being amortized the cost of doing computations into a loop that does few things, or "free" in the case of OooE CPUs with 3-4 instructions per clock.
* LUT setup would be on-demand: in cases with not random input data nodes with more than one child will be the common case, so initializing LUTs that are not going to be used would be wasting CPU cycles.

Before the tree search, a RLE attempt is done. That's almost "free" because accessing sequential data, so in worst case it acts as data prefetch.

### Memory usage reduction optimization: decoding

Decoding requires less memory than decoding because there is no need to guess node population, but a LZW code already has an associated output byte.

### Algorithm optimization: decoding

For LZW decoding, the tree LZW tree is reconstructed from received opcodes. A vector with the associattion between LZW code and associated character allows to reconstruct a pattern backwards (no optimization is done here). RLE opcodes mixed into the LZW stream are very fast because are implemented as a memset (1-byte patterns) and custom 3 and 4-byte memset functions.

### Next steps

- Uncompressible data handling.
- Speed-up LZW tree search on encoding.
- Add compressed data "cat" support.


# CacheSim – A Configurable Cache Simulator in C

This project implements a **single-level cache simulator** written in C, designed to simulate cache behavior under different configurations. It supports various cache sizes, associativities, and block sizes. The simulator processes memory access traces and provides detailed output on **cache hits, misses, replacements, and data values**.

---

## Features

- ✅ Configurable cache size, associativity, and block size
- ✅ **Write-back** and **write-allocate** policy
- ✅ **LRU replacement** within each set
- ✅ Bitwise address decomposition (no `%` or `math.h`)
- ✅ 16MB byte-addressable main memory simulation
- ✅ Detailed per-access output including evictions and dirty/clean status

---

## Usage

```bash
./cachesim <trace-file> <cache-size-kB> <associativity> <block-size>

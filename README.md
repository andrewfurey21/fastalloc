# fastalloc

Single header arena allocator. Includes an interface to allow for tuning, with options for alignment, nothrow, page size and memory pinning. Just copy and paste the header, and you're good to go.

For example, you can write a custom arena allocator that aligns elements to the cache line size and makes use of Linux 2mb huge pages.


```cpp
template <typename T, std::uint64_t NumElements>
using CustomArena =
  fastalloc::Arena<T,
                   NumElements,
                   std::hardware_destructive_interference_size, // Alignment
                   false, // Nothrow.
                   fastalloc::ARENA_PAGE_SIZES::MB_2, // Linux Huge page
                   true>; // Pinning

```

Note that to use the huge pages feature, you need to reserve huge pages by writing how many you want to `/proc/sys/vm/nr_hugepages`.

> Currently only works on Linux.

## Tests

To build tests install gtest from `libgtest-dev`. Then build with CMake.

## Helpful links

* https://thealexcons.github.io/articles/memory-allocators.html
* https://www.gingerbill.org/article/2019/02/01/memory-allocation-strategies-001/
* https://danluu.com/malloc-tutorial/
* https://antonz.org/allocators/
* https://people.freebsd.org/~jasone/jemalloc/bsdcan2006/jemalloc.pdf

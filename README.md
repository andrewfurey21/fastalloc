# fastalloc

Single header library with multiple fast standard library compatible memory allocators. Each allocator is templated to allow for tuning, including options for alignment, nothrow, page size and memory pinning. Just copy and paste a header, and you're good to go.

For example, you can write a custom arena allocator that aligns elements to the cache line size and make use of Linux 2mb huge pages. Note that to use the huge pages feature, you need to reserve huge pages by writing how many you want to `/proc/sys/vm/nr_hugepages`.

```cpp
template <typename T, std::uint64_t NumElements>
using CustomArean =
  fastalloc::Arena<T,
                   NumElements,
                   std::hardware_destructive_interference_size, // Alignment
                   false, // Nothrow.
                   fastalloc::ARENA_PAGE_SIZES::MB_2, // Linux Huge page
                   true>; // Pinning

```

> [!note] Currently only works on Linux.

- [ ] standard library container templated tests
- [ ] pool allocator
- [ ] freelist
- [ ] buddy
- [ ] slab
- [ ] segregated
- [ ] tracking

## features

- [x] Arena

## Tests

To build tests install gtest from `libgtest-dev`. Then build with CMake.

## Helpful links

* https://thealexcons.github.io/articles/memory-allocators.html
* https://www.gingerbill.org/article/2019/02/01/memory-allocation-strategies-001/
* https://danluu.com/malloc-tutorial/
* https://antonz.org/allocators/
* https://people.freebsd.org/~jasone/jemalloc/bsdcan2006/jemalloc.pdf

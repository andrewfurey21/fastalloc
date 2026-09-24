# fastalloc

single header library with multiple fast standard library compatible memory allocators

> [!note] Currently only works on Linux.

- [x] standard library allocators
  - [x] Allocator named requirement
  - [x] std::allocator
    - [x] rebind (in notes in std::allocator)
  - [x] std::allocator_traits
- [x] custom new/delete, new expression vs new call
- [ ] arena allocator
  - [ ] have a clearer purpose. how should copying data structures affect the allocator? idea: copying should not map more memory. uses the same memory, but becomes non-owning. i would like the arena to outlast the stack, i mean you could just allocate stack memory if that can't happen.so no stack alloc. maybe implement a stack-based allocator too. actually i want stack based, but maybe std::terminate or something if refcount > 1. pinned memory?
  - [ ] initial impl, works with different stdlib data structures
  - [ ] options for alignment, cache-aligned, huge pages, thread-safe (?, std::atomic)
- [ ] pool allocator
  - [ ] impl using page table-like indexing with tzcount
  - [ ] if tzcount not available, hackers delight impl
- [ ] freelist
- [ ] buddy
- [ ] slab
- [ ] segregated
- [ ] tracking

## notes

In C++, an allocator is a templated class that allocates and deallocates for a specific type T (QUESTION: how to manage std::vector<T*> that point to derived classes from bases with virtual methods).

stateful vs stateless allocators?

* Should have an `T *allocate(size_t n)` and `void deallocate(T *p, size_t n)`.
* `using value_type = T`
* if you change templates, you need to define a `struct rebind`



## links

* https://thealexcons.github.io/articles/memory-allocators.html
* https://www.gingerbill.org/article/2019/02/01/memory-allocation-strategies-001/
* https://danluu.com/malloc-tutorial/
* https://antonz.org/allocators/
* https://people.freebsd.org/~jasone/jemalloc/bsdcan2006/jemalloc.pdf

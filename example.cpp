#include <cstdint>
#include <iostream>
#include <cassert>
#include <new>

#include "arena_allocator.hpp"

template <typename T, std::uint64_t NumElements>
using CacheAligned2MBPageArena =
  fastalloc::Arena<T,
                   NumElements,
                   std::hardware_destructive_interference_size,
                   false,
                   fastalloc::ARENA_PAGE_SIZES::MB_2,
                   true>;

int main() {

  CacheAligned2MBPageArena<int, 1024> arena;

  // for (int i = 0; i < (2 * 1024 * 1024 - 64) / 64; i++) {
  //   int *x = arena.allocate(1);
  //   std::cout << i << " pointer: " << x << "\n" << std::flush;
  //   std::cout << "=====\n";
  // }

  // try {

  // int *x = arena.allocate(1);
  // } catch (const std::bad_alloc&) {
  //   std::cout << "caught a bad allocation\n";
  // }
  return 0;
}

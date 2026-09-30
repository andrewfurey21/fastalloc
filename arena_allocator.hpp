#ifndef __FASTALLOC_ARENA_ALLOCATOR_HPP
#define __FASTALLOC_ARENA_ALLOCATOR_HPP

#include <sys/mman.h>
#include <system_error>
#include <unistd.h>

#include <iostream>

namespace fastalloc {

#define u64 unsigned long long
#define u32 unsigned int
#define u8  unsigned char
#define i32 int

// TODO list
// 3. arena specific tests (clear, copy, destruct)
// 4. add license and push.

enum class ARENA_PAGE_SIZES : u32 {
  DEFAULT = 0,
  KB_16   = MAP_HUGE_16KB  | MAP_HUGETLB,
  KB_64   = MAP_HUGE_64KB  | MAP_HUGETLB,
  KB_512  = MAP_HUGE_512KB | MAP_HUGETLB,
  MB_1    = MAP_HUGE_1MB   | MAP_HUGETLB,
  MB_2    = MAP_HUGE_2MB   | MAP_HUGETLB,
  MB_8    = MAP_HUGE_8MB   | MAP_HUGETLB,
  MB_16   = MAP_HUGE_16MB  | MAP_HUGETLB,
  MB_32   = MAP_HUGE_32MB  | MAP_HUGETLB,
  MB_256  = MAP_HUGE_256MB | MAP_HUGETLB,
  MB_512  = MAP_HUGE_512MB | MAP_HUGETLB,
  GB_1    = MAP_HUGE_1GB   | MAP_HUGETLB,
  GB_2    = MAP_HUGE_2GB   | MAP_HUGETLB,
  GB_16   = MAP_HUGE_16GB  | MAP_HUGETLB,
};

inline const u64 round_up_to_even_pages(const u64 capacity_needed,
                                        const ARENA_PAGE_SIZES page_size) {
  constexpr u64 KB = 1024;
  u64 size = 0;
  switch (page_size) {
    case (ARENA_PAGE_SIZES::DEFAULT): size = sysconf(_SC_PAGE_SIZE); break;
    case (ARENA_PAGE_SIZES::KB_16):   size = 16 * KB; break;
    case (ARENA_PAGE_SIZES::KB_64):   size = 64 * KB; break;
    case (ARENA_PAGE_SIZES::KB_512):  size = 512 * KB; break;
    case (ARENA_PAGE_SIZES::MB_1):    size = 1 * KB * KB; break;
    case (ARENA_PAGE_SIZES::MB_2):    size = 2 * KB * KB; break;
    case (ARENA_PAGE_SIZES::MB_8):    size = 8 * KB * KB; break;
    case (ARENA_PAGE_SIZES::MB_16):   size = 16 * KB * KB; break;
    case (ARENA_PAGE_SIZES::MB_32):   size = 32 * KB * KB; break;
    case (ARENA_PAGE_SIZES::MB_256):  size = 256 * KB * KB; break;
    case (ARENA_PAGE_SIZES::MB_512):  size = 512 * KB * KB; break;
    case (ARENA_PAGE_SIZES::GB_1):    size = 1 * KB * KB * KB; break;
    case (ARENA_PAGE_SIZES::GB_2):    size = 2 * KB * KB * KB; break;
    case (ARENA_PAGE_SIZES::GB_16):   size = 16 * KB * KB * KB; break;
  }

  return size * ((capacity_needed + size - 1) / size);
}

template <typename T,
          u64 MaxNumElements,
          u64 Alignment = alignof(T),
          bool NoThrow  = false,
          ARENA_PAGE_SIZES HugePages = ARENA_PAGE_SIZES::DEFAULT,
          bool Pinned = false>
class Arena {
  static_assert((Alignment >= 1 && Alignment & (Alignment - 1)) == 0,
                "Alignment must be a power of 2 and greater than 0.");

public:

  // C++ named requirements: Allocator
  using pointer = T *;
  using const_pointer = const T *;
  using void_pointer = void *;
  using const_void_pointer = const void *;
  using value_type = T;
  using different_type = std::ptrdiff_t;
  using size_type = std::make_unsigned_t<different_type>;
  using propagate_on_container_move_assignment = std::false_type;
  using propagate_on_container_copy_assignment = std::true_type;
  using is_always_equal = std::false_type;

  template <typename U>
  struct rebind {
    using other = Arena<U,
                        MaxNumElements,
                        Alignment,
                        NoThrow,
                        HugePages,
                        Pinned>;
  };

  Arena() noexcept(NoThrow) :
    header(nullptr),
    total_bytes_allocated(0) {

    // const u64 size =
    //   round_up_to_even_pages(MaxNumElements * sizeof(T) + sizeof(ArenaHeader), HugePages);
    const u64 size =
      round_up_to_even_pages(MaxNumElements * size_of_each_allocation() +
                             sizeof(ArenaHeader), HugePages);

    void *start_virtual_memory = (void *)os_alloc(size);

    if (start_virtual_memory == nullptr) {
      if constexpr (NoThrow) return;
      else throw std::bad_alloc();
    }

    // Gets rounded up to an even number of pages. Needed for huge pages.
    total_bytes_allocated = size;

    header = new (start_virtual_memory) ArenaHeader();
  }

  Arena(const Arena& other) noexcept :
    header(other.header),
    total_bytes_allocated(other.total_bytes_allocated) {
    increase_ref_count();
  }

  Arena& operator=(const Arena& other) noexcept {
    if (this != &other) {
      this->decrease_ref_count_and_free();
      this->header = other.header;
      increase_ref_count();
    }
    return *this;
  }

  ~Arena() {
    decrease_ref_count_and_free();
  }

  T *allocate(u64 num_objects) noexcept(NoThrow) {
    if (header == nullptr) {
      if constexpr (NoThrow) return nullptr;
      else throw std::runtime_error("Arena::allocate(): trying to derefence an invalid arena (header is nulltr).");
    }
    if (num_objects == 0) return nullptr;

    const u64 new_allocation_size = num_objects * size_of_each_allocation();
    if (header->objects_allocated + num_objects > MaxNumElements) {
      if constexpr (NoThrow) return nullptr;
      else throw std::bad_alloc();
    }

    u8 *allocation = ((u8 *)(header)) + header->offset_from_header;
    header->offset_from_header += size_of_each_allocation() * num_objects;
    header->objects_allocated += num_objects;
    return (T *) allocation;
  }

  T *allocate(u64 num_objects, const_void_pointer cvp) noexcept(NoThrow) {
    // NOTE: could take advantage of cvp.
    return allocate(num_objects);
  }

  void deallocate(T *p, u64 num_objects) {}

  void clear() { header->offset_from_header = header + sizeof(ArenaHeader); }

  template <typename... Args>
  void construct(pointer p, Args... args) {
    new (p) T(std::forward<Args>(args)...);
  }

  void destroy(pointer p) { p->~T(); }

  bool operator==(const Arena& other) noexcept {
    return this->header == other.header;
  }

  bool operator!=(const Arena& other) noexcept { return !(*this == other); }

private:
  void *os_alloc(u64 num_bytes) noexcept {
    void *buffer = mmap(nullptr,
                        num_bytes,
                        PROT_READ | PROT_WRITE,
                        MAP_ANON | MAP_PRIVATE | (u32)HugePages,
                        0,
                        0);
    if constexpr (Pinned) {
      mlock(buffer, num_bytes);
    }

    if (buffer == MAP_FAILED) return nullptr;
    return buffer;
  }

  i32 os_dealloc(void *addr, u64 len) noexcept(NoThrow) {
    if (addr == nullptr) return 0;
    i32 ret = munmap(addr, len);

    if (ret != 0) {
      if constexpr (NoThrow) return -1;
      else throw std::system_error();
    }

    return 0;
  }

  constexpr u64 size_of_each_allocation() {
    return Alignment * ((sizeof(T) + Alignment - 1) / Alignment);
  }

  void increase_ref_count() {
    if (header != nullptr)
      header->ref_count++;
  }

  void decrease_ref_count_and_free() {
    if (header != nullptr) {
      header->ref_count--;
      if (header->ref_count == 0) {
        os_dealloc((void *)header, total_bytes_allocated);
      }
    }
  }

  struct alignas(Alignment) ArenaHeader {
    ArenaHeader() {
      ref_count = 1;
      offset_from_header = sizeof(ArenaHeader);
      objects_allocated = 0;
    }
    u64 ref_count;
    u64 offset_from_header;
    u64 objects_allocated;
  };

  ArenaHeader *header;
  u64 total_bytes_allocated;
};

#undef u64
#undef u32
#undef i32
#undef u8

}; // namespace fastalloc

#endif

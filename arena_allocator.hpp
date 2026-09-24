#ifndef __FASTALLOC_ARENA_ALLOCATOR_HPP
#define __FASTALLOC_ARENA_ALLOCATOR_HPP

#include <sys/mman.h>
#include <system_error>
#include <unistd.h>

namespace fastalloc {

#define u64 unsigned long long
#define u32 unsigned int
#define i32 int
#define u8  unsigned char

enum class ARENA_PAGE_SIZES : u32 {
  DEFAULT = 0, // NOTE: should be 4096 on Linux by default.
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

  return (capacity_needed + size - 1) / size;
}

// TODO list:
// tests!
template <typename T,
          u64 Capacity  = 128 * 1024 * 1024, // 128 MiB
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
  struct rebind { using other = Arena<U, Capacity, Alignment>; };

  Arena() noexcept(NoThrow) :
    header(nullptr) {

    void *start_virtual_memory = (void *)os_alloc(Capacity * sizeof(T));

    if constexpr (!NoThrow) {
      if (start_virtual_memory == nullptr) throw std::bad_alloc();
    } else {
      if (start_virtual_memory == nullptr) return;
    }

    header = new (start_virtual_memory) ArenaHeader();
    header->ref_count = 1;
    header->offset_from_header = sizeof(ArenaHeader);
  }

  Arena(const Arena& other) noexcept : header(other.header) {
    header->ref_count++;
  }

  Arena& operator=(const Arena& other) noexcept {
    if (this != &other) {
      this->decrease_ref_count_and_free();
      this->header = other.header;
      this->header->ref_count++;
    }
    return *this;
  }

  ~Arena() {
    decrease_ref_count_and_free();
  }

  T *allocate(u64 num_objects) noexcept(NoThrow) {
    if (num_objects == 0) return nullptr;

    if (header->offset_from_header >= Capacity) {
      if constexpr (NoThrow) return nullptr;
      else throw std::bad_alloc();
    }

    u8 *allocation = ((u8 *)(header)) + header->offset_from_header;
    header->offset_from_header += size_of_each_allocation() * num_objects;
    return (T *) allocation;
  }

  T *allocate(u64 num_objects, const_void_pointer cvp) noexcept(NoThrow) {
    // NOTE: could take advantage of cvp.
    return allocate(num_objects);
  }

  void deallocate(T *p, u64 num_objects) {}

  void clear() { header->offset_from_header = header + sizeof(ArenaHeader); }

  size_type max_size() {
    return Capacity - sizeof(ArenaHeader);
  }

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
    const u64 num_bytes_rounded_up =
      round_up_to_even_pages(num_bytes, HugePages);
    void *buffer = mmap(nullptr,
                        num_bytes_rounded_up,
                        PROT_READ | PROT_WRITE,
                        MAP_ANON | MAP_PRIVATE | (u32)HugePages,
                        0,
                        0);

    if constexpr (Pinned) {
      mlock(buffer, num_bytes_rounded_up);
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
    return sizeof(T) + Alignment - (sizeof(T) % Alignment);
  }

  void decrease_ref_count_and_free() {
    header->ref_count--;
    if (header->ref_count == 0) {
      os_dealloc((void *)header, Capacity);
    }
  }

  struct alignas(Alignment) ArenaHeader {
    u64 ref_count;
    u64 offset_from_header;
  };

  ArenaHeader *header;
};

#undef u64
#undef u32
#undef i32
#undef u8

}; // namespace fastalloc

#endif

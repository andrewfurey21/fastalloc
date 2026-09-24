

// Tests on most of the standard library containers, using GTest's
// Typed Test feature to generate identical test for each allocator,
// allocating non-trivial objects.

#include <gtest/gtest.h>

#include "../arena_allocator.hpp"

/*

vector: reserve, capacity, shrink_to_fit, clear, insert, emplace, erase, push_back, emplace_back, pop_back, resize

deque
string
list
forward_list
map
multimap
set
multiset
unordered_map
unordered_multimap
unordered_set
unordered_multiset

*/

#include <vector>

struct DummyType {
  DummyType() : x{1}, y{2}, z{3} { x++; }
  DummyType(const DummyType& other) : x{other.x}, y{other.y}, z{other.z} { y++; }
  DummyType(DummyType&& other) : x{other.x}, y{other.y}, z{other.z} { z++; }
  ~DummyType() {}
  int x;
  int y;
  int z;
};

// Test config.
using TestType = DummyType;
constexpr int NUM_ELEMENTS = 32 * 1024 * 1024;

template <typename T>
class AllocatorTests : public testing::Test {
public:
  template <typename U>
  using vector = std::vector<U, T>;
};

using BasicArena =
  fastalloc::Arena<TestType, NUM_ELEMENTS>;

using AllocatorTypes = ::testing::Types<BasicArena>;

TYPED_TEST_SUITE(AllocatorTests, AllocatorTypes);

// vector: shrink_to_fit, clear, insert, erase, resize, copy/move constructors/destructors .
TYPED_TEST(AllocatorTests, VectorTests) {
  typename TestFixture::template vector<TestType> v;

  v.reserve(1234);

  for (int i = 0; i < 20000; i+=2) {
    v.push_back(DummyType());
    v.emplace_back();
  }

  {
    typename TestFixture::template vector<TestType> v1 = v;
    v1.resize(30000);
  }

  typename TestFixture::template vector<TestType> v2 = std::move(v);

  // for (int i = 0; i < 10; i++) {
  //   v.emplace_back();
  // }



}

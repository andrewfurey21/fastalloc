

// Tests on most of the standard library containers methods that allocate memory, using GTest's
// Typed Test feature to generate identical test for each allocators.

// NOTE this doesn't test different alignments, since classes like std::vector
// will just do data[i] which will offset by alignof(T) instead of the
// preferred alignment, since it has no parameter to pass this in.


#include <gtest/gtest.h>

#include "../arena_allocator.hpp"

// Test config.
using namespace std; // Easy to swap.
constexpr int NUM_ELEMENTS = 32 * 1024 * 1024;

#include <vector>
#include <deque>

/*

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


struct DummyType {
  DummyType() : x{1}, y{2}, z{3} {}

  DummyType(int x, int y, int z) : x{x}, y{y}, z{z} {}

  DummyType(const DummyType& other) :
    x{other.x}, y{other.y}, z{other.z} {}

  DummyType(DummyType&& other) : x{other.x}, y{other.y}, z{other.z} {}

  bool operator==(const DummyType& other) const {
    return (x == other.x) && (y == other.y) && (z == other.z);
  }

  ~DummyType() {}

  int x;
  int y;
  int z;
};


template <typename T>
class AllocatorTests : public testing::Test {
public:
  template <typename U>
  using vector = vector<U, T>;
};

using BasicArena =
  fastalloc::Arena<DummyType, NUM_ELEMENTS>;
using HugePage2MBArena=
  fastalloc::Arena<DummyType,
                   NUM_ELEMENTS,
                   alignof(DummyType),
                   false,
                   fastalloc::ARENA_PAGE_SIZES::MB_2>;
using PinnedArena=
  fastalloc::Arena<DummyType,
                   NUM_ELEMENTS,
                   alignof(DummyType),
                   false,
                   fastalloc::ARENA_PAGE_SIZES::MB_2,
                   true>;

using AllocatorTypes = ::testing::Types<BasicArena,
                                        HugePage2MBArena,
                                        PinnedArena>;

TYPED_TEST_SUITE(AllocatorTests, AllocatorTypes);

TYPED_TEST(AllocatorTests, VectorPushBack) {
  typename TestFixture::template vector<DummyType> v;

  for (int i = 0; i < 10000; i++) {
    EXPECT_NO_THROW(v.push_back(DummyType(i, i+1, i+2)));
  }

  for (int i = 0;  i < 10000; i++) {
    EXPECT_EQ(v.at(i), DummyType(i, i+1, i+2));
  }
}

TYPED_TEST(AllocatorTests, VectorReserve) {
  typename TestFixture::template vector<DummyType> v;

  v.reserve(1234);
  for (int i = 0; i < 10000; i++) {
    EXPECT_NO_THROW(v.push_back(DummyType(i, i+1, i+2)));
  }

  for (int i = 0;  i < 10000; i++) {
    EXPECT_EQ(v[i], DummyType(i, i+1, i+2));
  }
}

TYPED_TEST(AllocatorTests, VectorShrinkToFit) {
  typename TestFixture::template vector<DummyType> v;

  for (int i = 0; i < 10000; i++) {
    EXPECT_NO_THROW(v.push_back(DummyType(i, i+1, i+2)));
  }

  v.shrink_to_fit();

  for (int i = 0;  i < 10000; i++) {
    EXPECT_EQ(v[i], DummyType(i, i+1, i+2));
  }

  EXPECT_EQ(v.capacity(), 10000);
}

TYPED_TEST(AllocatorTests, VectorClear) {
  typename TestFixture::template vector<DummyType> v;

  for (int i = 0; i < 10000; i++) {
    EXPECT_NO_THROW(v.push_back(DummyType(i, i+1, i+2)));
  }

  v.clear();

  for (int i = 0; i < 10000; i++) {
    EXPECT_NO_THROW(v.push_back(DummyType(i+1, i+2, i+3)));
  }

  for (int i = 0;  i < 10000; i++) {
    EXPECT_EQ(v[i], DummyType(i+1, i+2, i+3));
  }
}

TYPED_TEST(AllocatorTests, VectorResize) {
  typename TestFixture::template vector<DummyType> v;

  for (int i = 0; i < 10000; i++) {
    EXPECT_NO_THROW(v.push_back(DummyType(i, i+1, i+2)));
  }

  v.resize(1000);

  for (int i = 0;  i < v.size(); i++) {
    EXPECT_EQ(v[i], DummyType(i, i+1, i+2));
  }
}

TYPED_TEST(AllocatorTests, VectorCopy) {
  typename TestFixture::template vector<DummyType> v;

  for (int i = 0; i < 10000; i++) {
    EXPECT_NO_THROW(v.push_back(DummyType(i, i+1, i+2)));
  }

  {
    auto v1 = v;

    for (int i = 0; i < v1.size(); i++) {
      EXPECT_EQ(v1[i], DummyType(i, i+1, i+2));
    }
  }

  for (int i = 0; i < v.size(); i++) {
    EXPECT_EQ(v[i], DummyType(i, i+1, i+2));
  }
}

TYPED_TEST(AllocatorTests, VectorMove) {
  typename TestFixture::template vector<DummyType> v;

  for (int i = 0; i < 10000; i++) {
    EXPECT_NO_THROW(v.push_back(DummyType(i, i+1, i+2)));
  }

  {
    auto v1 = std::move(v);

    for (int i = 0; i < v1.size(); i++) {
      EXPECT_EQ(v1[i], DummyType(i, i+1, i+2));
    }
  }

  EXPECT_NO_THROW(v.reserve(1000));
}

TYPED_TEST(AllocatorTests, VectorSuccessfulMaxAllocation) {
  typename TestFixture::template vector<DummyType> v;

  EXPECT_NO_THROW(v.reserve(NUM_ELEMENTS));
}

TYPED_TEST(AllocatorTests, VectorFailureToAllocate) {
  typename TestFixture::template vector<DummyType> v;

  EXPECT_THROW(v.reserve(NUM_ELEMENTS + 1), std::bad_alloc);
}

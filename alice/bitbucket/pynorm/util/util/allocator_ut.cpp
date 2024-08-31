#include "allocator.h"

#include <library/cpp/testing/unittest/registar.h>

struct TIndexed {
    uint32_t index;
};

using TAlloc = TSmallTypedAllocator<TIndexed, 1>;

struct TAllocatorTest {
    void TestEmpty() {
        TAlloc alloc;
        UNIT_ASSERT(alloc.Storage.empty());
        UNIT_ASSERT(alloc.FreeIndices.empty());
    }
    void TestAllocOne() {
        TAlloc alloc;
        auto& elem = alloc.Alloc();
        UNIT_ASSERT(alloc.Storage.size() == 1);
        UNIT_ASSERT(alloc.FreeIndices.empty());
        UNIT_ASSERT(elem.index == 0);
    }
    void TestAllocTwo() {
        TAlloc alloc;
        auto& elem1 = alloc.Alloc();
        auto& elem2 = alloc.Alloc();
        UNIT_ASSERT(alloc.Storage.size() == 2);
        UNIT_ASSERT(alloc.FreeIndices.empty());
        UNIT_ASSERT(elem1.index == 0);
        UNIT_ASSERT(elem2.index == 1);
    }
    void TestAllocAndFree() {
        TAlloc alloc;
        auto& elem = alloc.Alloc();
        alloc.Free(elem);
        UNIT_ASSERT(alloc.Storage.size() == 1);
        UNIT_ASSERT(alloc.FreeIndices.size() == 1);
        UNIT_ASSERT(alloc.FreeIndices[0] == 0);
    }
    void TestAllocFreeAlloc() {
        TAlloc alloc;
        auto& elem = alloc.Alloc();
        alloc.Free(elem);
        auto& elem2 = alloc.Alloc();
        UNIT_ASSERT(alloc.Storage.size() == 1);
        UNIT_ASSERT(alloc.FreeIndices.empty());
        UNIT_ASSERT(&elem == &elem2);
    }
    void TestAllocTwoFreeTwo() {
        TAlloc alloc;
        auto& elem1 = alloc.Alloc();
        auto& elem2 = alloc.Alloc();
        alloc.Free(elem1);
        alloc.Free(elem2);
        UNIT_ASSERT(alloc.Storage.size() == 2);
        UNIT_ASSERT(alloc.FreeIndices.size() == 2);
        UNIT_ASSERT(alloc.FreeIndices[0] == 0);
        UNIT_ASSERT(alloc.FreeIndices[1] == 1);
    }
    void TestAllocTwoFreeTwoBackOrder() {
        TAlloc alloc;
        auto& elem1 = alloc.Alloc();
        auto& elem2 = alloc.Alloc();
        alloc.Free(elem2);
        alloc.Free(elem1);
        UNIT_ASSERT(alloc.Storage.size() == 2);
        UNIT_ASSERT(alloc.FreeIndices.size() == 2);
        UNIT_ASSERT(alloc.FreeIndices[0] == 1);
        UNIT_ASSERT(alloc.FreeIndices[1] == 0);
    }
};

Y_UNIT_TEST_SUITE(TAllocator) {
    Y_UNIT_TEST(TestAll) {
        TAllocatorTest test;
        test.TestEmpty();
        test.TestAllocOne();
        test.TestAllocTwo();
        test.TestAllocAndFree();
        test.TestAllocFreeAlloc();
        test.TestAllocTwoFreeTwo();
        test.TestAllocTwoFreeTwoBackOrder();
    }
}

#include "min_heap.h"

#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice;

namespace {
Y_UNIT_TEST_SUITE(MinHeapTests) {
    Y_UNIT_TEST(Smoke) {
        TMinHeap<int> heap;
        UNIT_ASSERT(heap.Empty());
        UNIT_ASSERT_VALUES_EQUAL(heap.Size(), 0);

        heap.Push(42);
        UNIT_ASSERT(!heap.Empty());
        UNIT_ASSERT_VALUES_EQUAL(heap.Size(), 1);
        UNIT_ASSERT_VALUES_EQUAL(heap.GetMin(), 42);

        heap.ExtractMin();
        UNIT_ASSERT(heap.Empty());
        UNIT_ASSERT_VALUES_EQUAL(heap.Size(), 0);
    }

    Y_UNIT_TEST(Sort) {
        TMinHeap<int> heap;

        heap.Push(3);
        UNIT_ASSERT_VALUES_EQUAL(heap.Size(), 1);
        UNIT_ASSERT_VALUES_EQUAL(heap.GetMin(), 3);

        heap.Emplace(10);
        UNIT_ASSERT_VALUES_EQUAL(heap.Size(), 2);
        UNIT_ASSERT_VALUES_EQUAL(heap.GetMin(), 3);

        heap.Push(1);
        UNIT_ASSERT_VALUES_EQUAL(heap.Size(), 3);
        UNIT_ASSERT_VALUES_EQUAL(heap.GetMin(), 1);

        heap.Emplace(3);
        UNIT_ASSERT_VALUES_EQUAL(heap.Size(), 4);
        UNIT_ASSERT_VALUES_EQUAL(heap.GetMin(), 1);

        heap.Push(0);
        UNIT_ASSERT_VALUES_EQUAL(heap.Size(), 5);
        UNIT_ASSERT_VALUES_EQUAL(heap.GetMin(), 0);

        // Heap contains now: 0, 1, 3, 3, 10.
        heap.ExtractMin();
        UNIT_ASSERT_VALUES_EQUAL(heap.Size(), 4);
        UNIT_ASSERT_VALUES_EQUAL(heap.GetMin(), 1);

        heap.ExtractMin();
        UNIT_ASSERT_VALUES_EQUAL(heap.Size(), 3);
        UNIT_ASSERT_VALUES_EQUAL(heap.GetMin(), 3);

        heap.ExtractMin();
        UNIT_ASSERT_VALUES_EQUAL(heap.Size(), 2);
        UNIT_ASSERT_VALUES_EQUAL(heap.GetMin(), 3);

        heap.ExtractMin();
        UNIT_ASSERT_VALUES_EQUAL(heap.Size(), 1);
        UNIT_ASSERT_VALUES_EQUAL(heap.GetMin(), 10);

        heap.ExtractMin();
        UNIT_ASSERT_VALUES_EQUAL(heap.Size(), 0);
    }
}
} // namespace

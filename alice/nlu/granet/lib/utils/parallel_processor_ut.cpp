#include <alice/nlu/granet/lib/utils/parallel_processor.h>
#include <library/cpp/iterator/zip.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace NGranet;

Y_UNIT_TEST_SUITE(TParallelProcessorWithOrderedPostprocess) {

    Y_UNIT_TEST(Init) {
        const TVector<int> inputs = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        const TVector<int> delays = {1, 20, 1, 1, 1, 1, 5, 1, 2, 3};
        TVector<int> outputs;

        THolder<IThreadPool> threadPool = CreateThreadPool(3);
        TParallelProcessorWithOrderedPostprocess processor(
            /* process= */ [](const std::pair<int, int>& data) {
                usleep(data.second * 10000);
                return data.first * 10;
            },
            /* postprocess= */ [&outputs](int data) {
                outputs.push_back(data);
            },
            /* threadPool= */ threadPool.Get(),
            /* resultQueueLimit= */ 5
        );
        for (const auto& [input, delay] : Zip(inputs, delays)) {
            processor.Push(std::make_pair(input, delay));
        }
        processor.Finalize();

        const TVector<int> expected = {0, 10, 20, 30, 40, 50, 60, 70, 80, 90};
        UNIT_ASSERT_EQUAL(outputs, expected);
    }
}

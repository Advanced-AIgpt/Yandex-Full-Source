#include <alice/nlu/libs/fst/archive_data_loader.h>
#include <alice/nlu/libs/fst/decoder.h>
#include <alice/nlu/libs/fst/fst_base.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/archive/yarchive.h>

#include <util/memory/blob.h>

namespace NAlice {

    extern "C" {
        extern const ui8 SoftFstData[];
        extern const ui32 SoftFstDataSize;
    }


    class TFstBaseTest final : public TTestBase {
    private:
        using TSelf = TFstBaseTest;
        UNIT_TEST_SUITE(TSelf);
        UNIT_TEST(TestParse);
        UNIT_TEST_SUITE_END();

    public:
        void TestParse() {
            auto reader = MakeHolder<TArchiveReader>(TBlob::NoCopy(SoftFstData, SoftFstDataSize));
            TFstDecoder decoder{TArchiveDataLoader{std::move(reader)}};
            TFstBase fstBase{std::move(decoder)};
            auto out = fstBase.Parse("такая простая строчка саша сашенька сашуля");
            UNIT_ASSERT_EQUAL(out.size(), 6u);
        }
    };

    UNIT_TEST_SUITE_REGISTRATION(TFstBaseTest);

} // namespace NAlice

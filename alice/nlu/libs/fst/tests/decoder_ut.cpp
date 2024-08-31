#include <alice/nlu/libs/fst/archive_data_loader.h>
#include <alice/nlu/libs/fst/decoder.h>
#include <alice/nlu/libs/fst/fst_base.h>

#include <util/memory/blob.h>

#include <library/cpp/archive/yarchive.h>
#include <library/cpp/testing/unittest/registar.h>

namespace NAlice {

    extern "C" {
        extern const ui8 SoftFstData[];
        extern const ui32 SoftFstDataSize;
    }


    class TFstDecoderTest final : public TTestBase {
    private:
        using TSelf = TFstDecoderTest;
        UNIT_TEST_SUITE(TSelf);
        UNIT_TEST(TestNormalize);
        UNIT_TEST(TestEmpty);
        UNIT_TEST_SUITE_END();

    public:
        void TestNormalize() {
            auto reader = MakeHolder<TArchiveReader>(TBlob::NoCopy(SoftFstData, SoftFstDataSize));
            TArchiveDataLoader loader{std::move(reader)};
            TFstDecoder decoder{loader};
            const auto& normalized = decoder.Normalize(TFstBase::PreNormalize("шла саша по шоссе и сосала сушку"));
            Cout << normalized << Endl;
            UNIT_ASSERT_STRINGS_EQUAL(normalized,  " |#шла  |SOFT.#саша  |#по  |#шоссе  |#и  |#сосала  |#сушку ");
        }

        void TestEmpty() {
            auto reader = MakeHolder<TArchiveReader>(TBlob::NoCopy(SoftFstData, SoftFstDataSize));
            TArchiveDataLoader loader{std::move(reader)};
            TFstDecoder decoder{loader};
            const auto& normalized = decoder.Normalize("");
            UNIT_ASSERT_STRINGS_EQUAL(normalized,  "");
        }

    };

    UNIT_TEST_SUITE_REGISTRATION(TFstDecoderTest);

} // namespace NAlice

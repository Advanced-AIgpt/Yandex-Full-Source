#include "common.h"

#include <alice/nlu/libs/fst/archive_data_loader.h>
#include <alice/nlu/libs/fst/fst_custom.h>

#include <library/cpp/testing/unittest/registar.h>

#include <algorithm>
#include <util/string/cast.h>

using namespace NAlice;
using namespace NAlice::NTestHelpers;

extern "C" {
    extern const ui8 SoftFstData[];
    extern const ui32 SoftFstDataSize;

    extern const ui8 AlbumFstData[];
    extern const ui32 AlbumFstDataSize;
}

Y_UNIT_TEST_SUITE(TFstCustomTests) {

    Y_UNIT_TEST(ParseAlbum) {
        TTestCase testCases[] = {
            {"как тебе kingdom come", {CreateEntity(1, 2, "ALBUM", "album"), CreateEntity(2, 4, "ALBUM", "album")}},
            {"включи englishman in new york", {CreateEntity(1, 5, "ALBUM", "album")}},
            {"поставь california dreaming", {CreateEntity(1, 3, "ALBUM", "album")}}
        };
        auto reader = MakeHolder<TArchiveReader>(TBlob::NoCopy(AlbumFstData, AlbumFstDataSize));
        TArchiveDataLoader loader{std::move(reader)};
        TFstCustom fstCustom{loader};
        for (const auto& testCase : testCases) {
            auto&& entities = fstCustom.Parse(ToString(testCase.Name));
            DropExcept("ALBUM", &entities);
            auto equal = std::equal(
                begin(entities), end(entities),
                begin(testCase.Entities), end(testCase.Entities),
                Eq
            );
            UNIT_ASSERT(equal);
        }
    }

    Y_UNIT_TEST(ParseAlbumEmpty) {
        auto reader = MakeHolder<TArchiveReader>(TBlob::NoCopy(AlbumFstData, AlbumFstDataSize));
        TArchiveDataLoader loader{std::move(reader)};
        TFstCustom fstCustom{loader};
        auto&& entities = fstCustom.Parse("");
        UNIT_ASSERT(entities.empty());
    }

    Y_UNIT_TEST(ParseSoft) {
        TTestCase testCases[] = {
            {"открой мне скайп", {CreateEntity(2, 3, "SOFT", "skype")}},
            {"скачать яндекс браузер", {CreateEntity(1, 3, "SOFT", "яндекс.браузер")}},
            {"яндекс браузер", {CreateEntity(0, 2, "SOFT", "яндекс.браузер")}}
        };
        auto reader = MakeHolder<TArchiveReader>(TBlob::NoCopy(SoftFstData, SoftFstDataSize));
        TArchiveDataLoader loader{std::move(reader)};
        TFstCustom fstCustom{loader};
        for (const auto& testCase : testCases) {
            auto&& entities = fstCustom.Parse(ToString(testCase.Name));
            DropExcept("SOFT", &entities);
            auto equal = std::equal(
                begin(entities), end(entities),
                begin(testCase.Entities), end(testCase.Entities),
                Eq
            );
            UNIT_ASSERT(equal);
        }
    }

}

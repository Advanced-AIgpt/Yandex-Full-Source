#include <library/cpp/testing/unittest/registar.h>

#include "track_quality_selector.h"

namespace NAlice::NHollywood::NMusic {

namespace {

bool operator==(const TDownloadInfoItem& a, const TDownloadInfoItem& b) {
    return a.BitrateInKbps == b.BitrateInKbps &&
           a.Codec == b.Codec &&
           a.Preview == b.Preview &&
           a.Gain == b.Gain &&
           a.Container == b.Container;
}

enum EDownloadItemOption {
    None = 0,
    Gain = 1,
    Preview = 2,
    HlsContainer = 4
};

Y_DECLARE_FLAGS(TDownloadItemOption, EDownloadItemOption)

TDownloadInfoItem Mp3(i32 bitrate, TDownloadItemOption opts = EDownloadItemOption::None) {
    return {
        EAudioCodec::MP3,
        bitrate,
        static_cast<bool>(opts & Gain),
        static_cast<bool>(opts & Preview),
        /* DownloadInfoUrl = */ "",
        /* Container = */ opts & HlsContainer ? EAudioContainer::HLS : EAudioContainer::RAW,
        /* ExpiringAtMs = */ 1653669451000,
    };
}

TDownloadInfoItem Aac(i32 bitrate, TDownloadItemOption opts = EDownloadItemOption::None) {
    return {
        EAudioCodec::AAC,
        bitrate,
        static_cast<bool>(opts & Gain),
        static_cast<bool>(opts & Preview),
        /* DownloadInfoUrl = */ "",
        /* Container = */ opts & HlsContainer ? EAudioContainer::HLS : EAudioContainer::RAW,
        /* ExpiringAtMs = */ 1653669451000,
    };
}

bool CheckMatch(TTrackQualitySelector sel, const TDownloadInfoOptions& info, const TDownloadInfoItem& match) {
    auto ptr = sel(info);
    return ptr && *ptr == match;
}

bool CheckNoMatch(TTrackQualitySelector sel, const TDownloadInfoOptions& info) {
    return !sel(info);
}

Y_UNIT_TEST_SUITE(TTrackQualitySelectorTest) {
    Y_UNIT_TEST(HighQualityTest) {
        THighQualitySelector sel;

        UNIT_ASSERT(CheckMatch(sel, {Mp3(192), Mp3(48), Aac(128), Mp3(320), Aac(192)},
                               Mp3(320)));

        UNIT_ASSERT(CheckMatch(sel, {Aac(192), Mp3(48), Aac(128), Aac(191)},
                               Aac(192)));

        UNIT_ASSERT(CheckMatch(sel, {Aac(256), Mp3(320)},
                               Mp3(320)));

        UNIT_ASSERT(CheckMatch(sel, {Mp3(192), Aac(192), Mp3(48), Aac(128), Aac(48)},
                               Aac(192)));

        UNIT_ASSERT(CheckMatch(sel, {Mp3(256), Aac(320), Mp3(48), Mp3(320)},
                               Aac(320)));

        UNIT_ASSERT(CheckMatch(sel, {Mp3(320), Mp3(320, Gain)}, Mp3(320)));

        UNIT_ASSERT(CheckMatch(sel, {Mp3(320, Gain), Mp3(320)},
                               Mp3(320)));

        UNIT_ASSERT(CheckMatch(sel, {Mp3(320, Gain), Aac(320, Gain), Mp3(320), Aac(320)},
                               Aac(320)));
    }

    Y_UNIT_TEST(HighQualityMp3OnlyTest) {
        THighQualitySelector sel;

        sel.SetAllowedCodecs(EAudioCodec::MP3);

        UNIT_ASSERT(CheckMatch(sel, {Mp3(192), Mp3(48), Aac(128), Mp3(320), Aac(192)},
                               Mp3(320)));

        UNIT_ASSERT(CheckMatch(sel, {Aac(192), Mp3(48), Aac(128), Aac(191)},
                               Mp3(48)));

        UNIT_ASSERT(CheckMatch(sel, {Aac(256), Mp3(320)},
                               Mp3(320)));

        UNIT_ASSERT(CheckMatch(sel, {Mp3(192), Aac(192), Mp3(48), Aac(128), Aac(48)},
                               Mp3(192)));

        UNIT_ASSERT(CheckMatch(sel, {Mp3(256), Aac(320), Mp3(48), Mp3(320)},
                               Mp3(320)));

        UNIT_ASSERT(CheckMatch(sel, {Mp3(320), Mp3(320, Gain)},
                               Mp3(320)));

        UNIT_ASSERT(CheckNoMatch(sel, {Aac(320), Aac(320, Gain)}));

        UNIT_ASSERT(CheckMatch(sel, {Mp3(320, Gain), Mp3(320)},
                               Mp3(320)));

        UNIT_ASSERT(CheckMatch(sel, {Mp3(320, Gain), Aac(320, Gain), Mp3(320), Aac(320)},
                               Mp3(320)));
    }

    Y_UNIT_TEST(HighQualityAacOnlyTest) {
        THighQualitySelector sel;

        sel.SetAllowedCodecs(EAudioCodec::AAC);

        UNIT_ASSERT(CheckMatch(sel, {Mp3(192), Mp3(48), Aac(128), Mp3(320), Aac(192)},
                               Aac(192)));

        UNIT_ASSERT(CheckMatch(sel, {Aac(192), Mp3(48), Aac(128), Aac(191)},
                               Aac(192)));

        UNIT_ASSERT(CheckMatch(sel, {Aac(256), Mp3(320)},
                               Aac(256)));

        UNIT_ASSERT(CheckMatch(sel, {Mp3(192), Aac(192), Mp3(48), Aac(128), Aac(48)},
                               Aac(192)));

        UNIT_ASSERT(CheckMatch(sel, {Mp3(256), Aac(320), Mp3(48), Mp3(320)},
                               Aac(320)));

        UNIT_ASSERT(CheckNoMatch(sel, {Mp3(320, Gain), Mp3(320)}));

        UNIT_ASSERT(CheckMatch(sel, {Mp3(320, Gain), Aac(320, Gain), Mp3(320), Aac(320)},
                               Aac(320)));
    }

    Y_UNIT_TEST(HighQualityPreviewTest) {
        THighQualitySelector sel;

        sel.SetAllowPreview(false);

        UNIT_ASSERT(CheckMatch(sel, {Mp3(320, Preview), Mp3(320)},
                               Mp3(320)));

        UNIT_ASSERT(CheckMatch(sel, {Mp3(320), Mp3(320, Preview)},
                               Mp3(320)));

        UNIT_ASSERT(CheckNoMatch(sel, {Mp3(128, Preview), Aac(128, Preview)}));

        sel.SetAllowPreview(true);

        UNIT_ASSERT(CheckMatch(sel, {Mp3(320, Preview), Mp3(256)},
                               Mp3(320, Preview)));

        UNIT_ASSERT(CheckMatch(sel, {Mp3(128, Preview), Aac(128, Preview)},
                               Aac(128, Preview)));
    }

    Y_UNIT_TEST(HighQualityDesiredBitrateTest) {
        THighQualitySelector sel;
        sel.SetDesiredBitrateInKbps(192);

        // desired bitrate is found (choose this)
        UNIT_ASSERT(CheckMatch(sel, {Mp3(192), Mp3(48), Aac(128), Mp3(320), Aac(192)},
                               Aac(192)));

        UNIT_ASSERT(CheckMatch(sel, {Aac(192), Mp3(48), Aac(128), Aac(191)},
                               Aac(192)));

        UNIT_ASSERT(CheckMatch(sel, {Mp3(320), Aac(320), Mp3(128), Mp3(192)},
                               Mp3(192)));

        // desired bitrate is not found (choose default)
        UNIT_ASSERT(CheckMatch(sel, {Mp3(256), Aac(320), Mp3(48), Mp3(320)},
                               Aac(320)));

        UNIT_ASSERT(CheckMatch(sel, {Mp3(256), Aac(320), Mp3(48), Mp3(320)},
                               Aac(320)));
    }

    Y_UNIT_TEST(HlsContainerTest) {
        THighQualitySelector sel;
        sel.SetAllowedContainer(EAudioContainer::HLS);
        UNIT_ASSERT(CheckMatch(sel, {Mp3(192), Mp3(48), Aac(128), Mp3(320), Aac(192),
                                     Mp3(192, HlsContainer), Mp3(48, HlsContainer), Aac(128, HlsContainer),
                                     Mp3(320, HlsContainer), Aac(192, HlsContainer)},
                               Mp3(320, HlsContainer)));

    }

};

}  // namespace

}  // namespace NAlice::NHollywood::NMusic

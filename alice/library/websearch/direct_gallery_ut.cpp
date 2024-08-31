#include "direct_gallery.h"

#include <alice/library/experiments/flags.h>

#include <library/cpp/testing/unittest/registar.h>

namespace {

using namespace NAlice;
using namespace NAlice::NDirectGallery;

Y_UNIT_TEST_SUITE(ClientFeatures) {
    Y_UNIT_TEST(CanShowDirectGalery) {
        {
            TClientInfoProto proto;
            proto.SetAppId("ru.yandex.searchplugin");
            const TRawExpFlags emptyFlags;
            TClientFeatures features{proto, emptyFlags};
            UNIT_ASSERT(CanShowDirectGallery(features));
        }
        {
            TClientInfoProto proto;
            proto.SetAppId("ru.yandex.searchplugin");
            const TRawExpFlags flags{{TString{NExperiments::WEBSEARCH_DISABLE_DIRECT_GALLERY}, "1"}};
            TClientFeatures features{proto, flags};
            UNIT_ASSERT(!CanShowDirectGallery(features));
        }
        {
            TClientInfoProto proto;
            proto.SetAppId("ru.quasar.app");
            const TRawExpFlags emptyFlags;
            TClientFeatures features{proto, emptyFlags};
            UNIT_ASSERT(!CanShowDirectGallery(features));
        }
        {
            TClientInfoProto proto;
            proto.SetAppId("ru.quasar.app");
            const TRawExpFlags flags{{TString{NExperiments::WEBSEARCH_ENABLE_DIRECT_GALLERY}, "1"}};
            TClientFeatures features{proto, flags};
            UNIT_ASSERT(CanShowDirectGallery(features));
        }
    }
}

} // namespace

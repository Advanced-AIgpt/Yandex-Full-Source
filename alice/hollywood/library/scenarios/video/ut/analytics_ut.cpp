#include <alice/hollywood/library/scenarios/video/analytics.h>

#include <google/protobuf/text_format.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywoodFw::NVideo {

const TString SEARCH_DATA_STRING = R"(
Galleries {
  BasicCarousel {
    Items {
      SearchVideoItem {
        Id: "2ZBaDCXd4tM"
        ContentType: "video"
        Title: "\320\237\321\200\320\270\320\272\320\276\320\273\321\213 \320\241 \320\232\320\276\321\210\320\272\320\260\320\274\320\270 \342\204\226494 \320\241\320\274\320\265\321\210\320\275\321\213\320\265 \320\232\320\276\321\202\321\213 \320\230 \320\232\320\276\321\202\321\217\321\202\320\260!  \320\241\320\260\320\274\321\213\320\265 \320\234\320\270\320\273\321\213\320\265 \320\230 \320\237\321\200\320\270\320\272\320\276\320\273\321\214\320\275\321\213\320\265!"
        PlayerId: "youtube"
        Hosting: "youtube.com"
        EmbedUri: "//www.youtube.com/embed/2ZBaDCXd4tM?autoplay=1&amp;enablejsapi=1&amp;wmode=opaque"
        Duration: 652
      }
    }
    Items {
      SearchVideoItem {
        Id: "gejCc5CcPGM"
        ContentType: "video"
        Title: "\320\232\320\276\321\210\320\272\320\270 2021 \320\241\320\274\320\265\321\210\320\275\321\213\320\265 \320\232\320\236\320\242\320\253 2021 \320\232\320\276\321\210\320\272\320\270 \320\237\321\200\320\270\320\272\320\276\320\273\321\213 \320\241 \320\232\320\276\321\202\320\270\320\272\320\260\320\274\320\270 \320\230 \320\232\320\276\321\210\320\272\320\260\320\274\320\270 Funny Cats \320\227\320\220 \320\235\320\276\321\217\320\261\321\200\321\214 2021"
        PlayerId: "youtube"
        Hosting: "youtube.com"
        EmbedUri: "//www.youtube.com/embed/gejCc5CcPGM?autoplay=1&amp;enablejsapi=1&amp;wmode=opaque"
        Duration: 607
      }
    }
    Items {
      SearchVideoItem {
        Id: "oLiRWE1fnRA"
        ContentType: "video"
        Title: "\320\241\320\274\320\265\321\210\320\275\321\213\320\265 \320\272\320\276\321\210\320\272\320\270 2021 \320\270 \320\264\321\200\321\203\320\263\320\270\320\265 \320\266\320\270\320\262\320\276\321\202\320\275\321\213\320\265/ 10 \320\274\320\270\320\275\321\203\321\202 \321\201\320\274\320\265\321\205\320\260/\321\201\320\274\320\265\321\210\320\275\321\213\320\265 \320\266\320\270\320\262\320\276\321\202\320\275\321\213\320\265 2021/\320\273\321\203\321\207\321\210\320\270\320\265..."
        PlayerId: "youtube"
        Hosting: "youtube.com"
        EmbedUri: "//www.youtube.com/embed/oLiRWE1fnRA?autoplay=1&amp;enablejsapi=1&amp;wmode=opaque"
        Duration: 606
      }
    }
    Items {
      SearchVideoItem {
        Id: "dhDi0CJN8FE"
        ContentType: "video"
        Title: "\320\241\320\274\320\265\321\210\320\275\321\213\320\265 \320\262\320\270\320\264\320\265\320\276 - \320\232\320\276\321\210\320\272\320\270 2021\360\237\230\202 \320\241\320\274\320\265\321\210\320\275\321\213\320\265 \320\272\320\276\321\202\321\213 \320\277\321\200\320\270\320\272\320\276\320\273\321\213 \321\201 \320\272\320\276\321\202\320\260\320\274\320\270 \320\264\320\276 \321\201\320\273\320\265\320\267 \342\200\223 \320\241\320\274\320\265\321\210\320\275\321\213\320\265 \320\272\320\276\321\210\320\272\320\270..."
        PlayerId: "youtube"
        Hosting: "youtube.com"
        EmbedUri: "//www.youtube.com/embed/dhDi0CJN8FE?autoplay=1&amp;enablejsapi=1&amp;wmode=opaque"
        Duration: 486
      }
    }
    Items {
      SearchVideoItem {
        Id: "3ngjmr31F0I"
        ContentType: "video"
        Title: "\320\241\320\274\320\265\321\210\320\275\321\213\320\265 \320\272\320\276\321\210\320\272\320\270 2022 \320\270 \320\264\321\200\321\203\320\263\320\270\320\265 \320\266\320\270\320\262\320\276\321\202\320\275\321\213\320\265/ 10 \320\274\320\270\320\275\321\203\321\202 \321\201\320\274\320\265\321\205\320\260/\321\201\320\274\320\265\321\210\320\275\321\213\320\265 \320\266\320\270\320\262\320\276\321\202\320\275\321\213\320\265 2022/\320\273\321\203\321\207\321\210\320\270\320\265..."
        PlayerId: "youtube"
        Hosting: "youtube.com"
        EmbedUri: "//www.youtube.com/embed/3ngjmr31F0I?autoplay=1&amp;enablejsapi=1&amp;wmode=opaque"
        Duration: 636
      }
    }
    Items {
      SearchVideoItem {
        Id: "BdsyQU6lkJI"
        ContentType: "video"
        Title: "\320\257 \320\240\320\226\320\220\320\233 \320\224\320\236 \320\241\320\233\320\225\320\227\360\237\230\202 \320\241\320\274\320\265\321\210\320\275\321\213\320\265 \320\262\320\270\320\264\320\265\320\276  \320\277\321\200\320\270\320\272\320\276\320\273\321\213 \321\201 \320\272\320\276\321\202\320\260\320\274\320\270 \320\270 \321\201\320\276\320\261\320\260\320\272\320\260\320\274\320\270, \320\232\320\236\320\242\320\253 \320\237\321\200\320\270\320\272\320\276\320\273\321\213 #2 /Funny..."
        PlayerId: "youtube"
        Hosting: "youtube.com"
        EmbedUri: "//www.youtube.com/embed/BdsyQU6lkJI?autoplay=1&amp;enablejsapi=1&amp;wmode=opaque"
        Duration: 616
      }
    }
    Items {
      SearchVideoItem {
        Id: "nA0AXUOugEc"
        ContentType: "video"
        Title: "\320\232\320\236\320\242\320\253 \320\237\321\200\320\270\320\272\320\276\320\273\321\213 \321\201 \320\232\320\276\321\202\320\260\320\274\320\270 \320\270 \320\232\320\276\321\210\320\272\320\260\320\274\320\270 2020 \320\232\320\276\321\210\320\272\320\270 \320\241\320\274\320\265\321\210\320\275\321\213\320\265 \320\232\320\276\321\202\321\213 Funny Cats"
        PlayerId: "youtube"
        Hosting: "youtube.com"
        EmbedUri: "//www.youtube.com/embed/nA0AXUOugEc?autoplay=1&amp;enablejsapi=1&amp;wmode=opaque"
        Duration: 644
      }
    }
  }
})";

Y_UNIT_TEST_SUITE(Analytics) {

    Y_UNIT_TEST(TestSearchResultAnalytics) {
        TTvSearchResultData searchData;
        google::protobuf::TextFormat::ParseFromString(SEARCH_DATA_STRING, &searchData);
        const auto& obj = NHollywood::NVideo::GetAnalyticsObjectForTvSearch(searchData);

        UNIT_ASSERT(obj.HasVideoSearchGalleryScreen());
        UNIT_ASSERT_EQUAL(obj.GetVideoSearchGalleryScreen().GetItems().size(), 7);
    }
} // Y_UNIT_TEST_SUITE

} // namespace NAlice::NHollywoodFw::NRandomNumber

#include "yavideo_utils.h"

#include <alice/library/unittest/ut_helpers.h>

#include <library/cpp/testing/unittest/registar.h>

#include <library/cpp/scheme/domscheme_traits.h>
#include <library/cpp/scheme/scheme.h>

using namespace NBASS::NVideo;
using namespace NVideoCommon;

namespace {

constexpr TStringBuf ERROR_RESPONSE = R"({
                                            "searchdata" : {
                                               "ban" : {
                                                  "RTBF" : null,
                                                  "antipirate" : 0,
                                                  "legal" : 0,
                                                  "rkn" : null
                                               },
                                               "clips" : [],
                                               "err_code" : 15,
                                               "err_text" : "Искомая комбинация слов нигде не встречается",
                                            }
                                         }
)";

constexpr TStringBuf EMPTY_RESPONSE = R"({
                                            "searchdata" : {
                                               "ban" : {
                                                  "RTBF" : null,
                                                  "antipirate" : 0,
                                                  "legal" : 0,
                                                  "rkn" : null
                                               },
                                               "clips" : [],
                                            }
                                         }
)";

constexpr TStringBuf GOOD_RESPONSE = R"({
                                           "searchdata" : {
                                              "ban" : {
                                                 "RTBF" : null,
                                                 "antipirate" : 0,
                                                 "legal" : 0,
                                                 "rkn" : null
                                              },
                                              "clips" : [
                                                 {
                                                    "thmb_href" : "thumbnail",
                                                    "title" : "SomeTitle",
                                                    "url" : "http://www.youtube.com/watch?v=8K8ClHoZzHw",
                                                    "PlayerId" : "youtube",
                                                    "HtmlAutoplayVideoPlayer" : "player"
                                                 }
                                              ]
                                           }
                                        }
)";

constexpr TStringBuf EXPECTED_GOOD_GALLERY = R"(
                                                   {
                                                     "items" : [
                                                       {
                                                         "available" : 1,
                                                         "debug_info" : {
                                                             "web_page_url" : "https://www.youtube.com/watch?v=8K8ClHoZzHw"
                                                         },
                                                         "description" : "",
                                                         "duration" : 0,
                                                         "name" : "SomeTitle",
                                                         "play_uri" : "youtube://8K8ClHoZzHw",
                                                         "provider_item_id" : "8K8ClHoZzHw",
                                                         "provider_name" : "youtube",
                                                         "source": "SOURCE",
                                                         "source_host" : "www.youtube.com",
                                                         "thumbnail_url_16x9" : "thumbnail&w=504&h=284",
                                                         "thumbnail_url_16x9_small" : "thumbnail&w=88&h=48",
                                                         "type" : "video"
                                                       }
                                                     ]
                                                   }
)";

class TFakeYaVideoDelegate final : public TYaVideoContentGetterDelegate {
public:
    NHttpFetcher::TRequestPtr
    AttachProviderRequest(NHttpFetcher::IMultiRequest::TRef /* multiRequest */) const override {
        return {};
    }

    bool IsSmartSpeaker() const override {
        return true;
    }

    bool IsTvDevice() const override {
        return false;
    }

    bool HasExpFlag(TStringBuf /* name */) const override {
        return false;
    }

    const TString& UserTld() const override {
        return FakeTld;
    }

    void FillCgis(TCgiParameters& /* cgis */) const override {
    }

    bool SupportsBrowserVideoGallery() const override {
        return false;
    }

    void FillTunnellerResponse(const NSc::TValue& /* jsonData */) const override {
    }

    void FillRequestForAnalyticsInfo(const TString& /* request */, const TString& /* text */, const ui32 /*code*/, const bool /*success*/) override {
    }

private:
    TString FakeTld;
};

Y_UNIT_TEST_SUITE(TYaVideoUtilsTest) {
    Y_UNIT_TEST(ParseJsonResponse) {
        const TStringBuf defaultSource = "SOURCE";
        TFakeYaVideoDelegate delegate;
        {
            TVideoGallery gallery;
            NVideoCommon::TResult result = ParseJsonResponse(NSc::TValue::FromJson(ERROR_RESPONSE), delegate,
                                                             &gallery.Scheme(), defaultSource);
            UNIT_ASSERT(result);
            UNIT_ASSERT_VALUES_EQUAL(result->Msg, "yavideo json returned an error: code 15, text:"
                                                              " Искомая комбинация слов нигде не встречается");
        }
        {
            TVideoGallery gallery;
            NVideoCommon::TResult result = ParseJsonResponse(NSc::TValue::FromJson(EMPTY_RESPONSE), delegate,
                                                             &gallery.Scheme(), defaultSource);
            UNIT_ASSERT(!result);
        }
        {
            TVideoGallery gallery;
            NVideoCommon::TResult result = ParseJsonResponse(NSc::TValue::FromJson(GOOD_RESPONSE), delegate,
                                                             &gallery.Scheme(), defaultSource);
            UNIT_ASSERT(!result);
            UNIT_ASSERT(
                NTestingHelpers::EqualJson(*gallery->GetRawValue(), NSc::TValue::FromJson(EXPECTED_GOOD_GALLERY)));
        }
    }
}
} // namespace

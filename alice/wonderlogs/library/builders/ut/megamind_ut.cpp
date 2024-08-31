#include <alice/wonderlogs/library/builders/megamind.h>

#include <alice/megamind/protos/speechkit/directives.pb.h>
#include <alice/megamind/protos/speechkit/request.pb.h>
#include <alice/megamind/protos/speechkit/response.pb.h>

#include <library/cpp/testing/unittest/registar.h>

namespace {

using namespace NAlice::NWonderlogs;

Y_UNIT_TEST_SUITE(Megamind) {
    Y_UNIT_TEST(PreferableRequestResponse1) {
        TMaybe<TMegamindPrepared::TMegamindRequestResponse> successfulMegamindRequestResponse;
        TMaybe<TUniproxyPrepared> successfulUniproxyPrepared;
        TMegamindPrepared::TMegamindRequestResponse megamindRequestResponse;

        UNIT_ASSERT(PreferableRequestResponse(successfulUniproxyPrepared, successfulMegamindRequestResponse,
                                              megamindRequestResponse));
    }

    Y_UNIT_TEST(PreferableRequestResponse2) {
        TMaybe<TMegamindPrepared::TMegamindRequestResponse> successfulMegamindRequestResponse =
            TMegamindPrepared::TMegamindRequestResponse();
        successfulMegamindRequestResponse->MutableSpeechKitRequest()
            ->MutableRequest()
            ->MutableAdditionalOptions()
            ->SetServerTimeMs(1);

        TMaybe<TUniproxyPrepared> successfulUniproxyPrepared;

        TMegamindPrepared::TMegamindRequestResponse megamindRequestResponse;
        megamindRequestResponse.MutableSpeechKitRequest()
            ->MutableRequest()
            ->MutableAdditionalOptions()
            ->SetServerTimeMs(2);

        UNIT_ASSERT(PreferableRequestResponse(successfulUniproxyPrepared, successfulMegamindRequestResponse,
                                              megamindRequestResponse));
    }

    Y_UNIT_TEST(PreferableRequestResponse3) {
        const TString LOL = "lol";
        const TString KEK = "kek";

        TMaybe<TMegamindPrepared::TMegamindRequestResponse> successfulMegamindRequestResponse =
            TMegamindPrepared::TMegamindRequestResponse();
        successfulMegamindRequestResponse->MutableSpeechKitRequest()
            ->MutableRequest()
            ->MutableAdditionalOptions()
            ->SetServerTimeMs(1);
        successfulMegamindRequestResponse->SetResponseId(LOL);

        TMaybe<TUniproxyPrepared> successfulUniproxyPrepared = TUniproxyPrepared();
        successfulUniproxyPrepared->SetMegamindResponseId(LOL);

        TMegamindPrepared::TMegamindRequestResponse megamindRequestResponse;
        megamindRequestResponse.MutableSpeechKitRequest()
            ->MutableRequest()
            ->MutableAdditionalOptions()
            ->SetServerTimeMs(2);

        megamindRequestResponse.SetResponseId(KEK);

        UNIT_ASSERT(!PreferableRequestResponse(successfulUniproxyPrepared, successfulMegamindRequestResponse,
                                               megamindRequestResponse));
    }

    Y_UNIT_TEST(PreferableRequestResponse4) {
        const TString LOL = "lol";
        const TString KEK = "kek";

        TMaybe<TMegamindPrepared::TMegamindRequestResponse> successfulMegamindRequestResponse =
            TMegamindPrepared::TMegamindRequestResponse();
        successfulMegamindRequestResponse->MutableSpeechKitRequest()
            ->MutableRequest()
            ->MutableAdditionalOptions()
            ->SetServerTimeMs(1);
        successfulMegamindRequestResponse->SetResponseId(KEK);

        TMaybe<TUniproxyPrepared> successfulUniproxyPrepared = TUniproxyPrepared();
        successfulUniproxyPrepared->SetMegamindResponseId(LOL);

        TMegamindPrepared::TMegamindRequestResponse megamindRequestResponse;
        megamindRequestResponse.MutableSpeechKitRequest()
            ->MutableRequest()
            ->MutableAdditionalOptions()
            ->SetServerTimeMs(2);

        megamindRequestResponse.SetResponseId(LOL);

        UNIT_ASSERT(PreferableRequestResponse(successfulUniproxyPrepared, successfulMegamindRequestResponse,
                                              megamindRequestResponse));
    }

    Y_UNIT_TEST(PreferableRequestResponse5) {
        const TString LOL = "lol";
        const TString KEK = "kek";

        TMaybe<TMegamindPrepared::TMegamindRequestResponse> successfulMegamindRequestResponse =
            TMegamindPrepared::TMegamindRequestResponse();
        successfulMegamindRequestResponse->MutableSpeechKitRequest()
            ->MutableRequest()
            ->MutableAdditionalOptions()
            ->SetServerTimeMs(1);
        successfulMegamindRequestResponse->SetResponseId(KEK);

        TMaybe<TUniproxyPrepared> successfulUniproxyPrepared = TUniproxyPrepared();
        successfulUniproxyPrepared->SetMegamindResponseId(LOL);

        TMegamindPrepared::TMegamindRequestResponse megamindRequestResponse;
        megamindRequestResponse.MutableSpeechKitRequest()
            ->MutableRequest()
            ->MutableAdditionalOptions()
            ->SetServerTimeMs(2);

        megamindRequestResponse.SetResponseId(LOL);
        megamindRequestResponse.MutableSpeechKitResponse()->MutableResponse()->AddDirectives()->SetName("defer_apply");

        UNIT_ASSERT(PreferableRequestResponse(successfulUniproxyPrepared, successfulMegamindRequestResponse,
                                              megamindRequestResponse));
    }

    Y_UNIT_TEST(PreferableRequestResponse6) {
        const TString LOL = "lol";
        const TString KEK = "kek";

        TMaybe<TMegamindPrepared::TMegamindRequestResponse> successfulMegamindRequestResponse =
            TMegamindPrepared::TMegamindRequestResponse();
        successfulMegamindRequestResponse->MutableSpeechKitRequest()
            ->MutableRequest()
            ->MutableAdditionalOptions()
            ->SetServerTimeMs(1);
        successfulMegamindRequestResponse->SetResponseId(LOL);

        TMaybe<TUniproxyPrepared> successfulUniproxyPrepared = TUniproxyPrepared();
        successfulUniproxyPrepared->SetMegamindResponseId(LOL);

        TMegamindPrepared::TMegamindRequestResponse megamindRequestResponse;
        megamindRequestResponse.MutableSpeechKitRequest()
            ->MutableRequest()
            ->MutableAdditionalOptions()
            ->SetServerTimeMs(2);

        megamindRequestResponse.SetResponseId(KEK);
        megamindRequestResponse.MutableSpeechKitResponse()->MutableResponse()->AddDirectives()->SetName("defer_apply");

        UNIT_ASSERT(!PreferableRequestResponse(successfulUniproxyPrepared, successfulMegamindRequestResponse,
                                               megamindRequestResponse));
    }

    Y_UNIT_TEST(PreferableRequestResponse7) {
        TMaybe<TMegamindPrepared::TMegamindRequestResponse> successfulMegamindRequestResponse =
            TMegamindPrepared::TMegamindRequestResponse();
        successfulMegamindRequestResponse->MutableSpeechKitRequest()
            ->MutableRequest()
            ->MutableAdditionalOptions()
            ->SetServerTimeMs(1);

        TMaybe<TUniproxyPrepared> successfulUniproxyPrepared;

        TMegamindPrepared::TMegamindRequestResponse megamindRequestResponse;
        megamindRequestResponse.MutableSpeechKitRequest()
            ->MutableRequest()
            ->MutableAdditionalOptions()
            ->SetServerTimeMs(1);

        megamindRequestResponse.MutableSpeechKitResponse()->MutableResponse()->AddDirectives()->SetName("defer_apply");

        UNIT_ASSERT(!PreferableRequestResponse(successfulUniproxyPrepared, successfulMegamindRequestResponse,
                                               megamindRequestResponse));
    }

    Y_UNIT_TEST(PreferableRequestResponse8) {
        TMaybe<TMegamindPrepared::TMegamindRequestResponse> successfulMegamindRequestResponse =
            TMegamindPrepared::TMegamindRequestResponse();
        successfulMegamindRequestResponse->MutableSpeechKitRequest()
            ->MutableRequest()
            ->MutableAdditionalOptions()
            ->SetServerTimeMs(1);
        successfulMegamindRequestResponse->MutableSpeechKitResponse()->MutableResponse()->AddDirectives()->SetName(
            "defer_apply");

        TMaybe<TUniproxyPrepared> successfulUniproxyPrepared;

        TMegamindPrepared::TMegamindRequestResponse megamindRequestResponse;
        megamindRequestResponse.MutableSpeechKitRequest()
            ->MutableRequest()
            ->MutableAdditionalOptions()
            ->SetServerTimeMs(1);

        UNIT_ASSERT(PreferableRequestResponse(successfulUniproxyPrepared, successfulMegamindRequestResponse,
                                              megamindRequestResponse));
    }

    Y_UNIT_TEST(PreferableRequestResponse9) {
        TMaybe<TMegamindPrepared::TMegamindRequestResponse> successfulMegamindRequestResponse;
        TMaybe<TUniproxyPrepared> successfulUniproxyPrepared;
        TMegamindPrepared::TMegamindRequestResponse megamindRequestResponse;
        megamindRequestResponse.MutableSpeechKitResponse()->MutableResponse()->AddDirectives()->SetName("defer_apply");

        UNIT_ASSERT(PreferableRequestResponse(successfulUniproxyPrepared, successfulMegamindRequestResponse,
                                              megamindRequestResponse));
    }
}

} // namespace

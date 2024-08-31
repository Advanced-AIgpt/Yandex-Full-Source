#include <alice/cuttlefish/library/cuttlefish/megamind/utils.h>

#include <alice/megamind/protos/common/events.pb.h>

#include <library/cpp/testing/unittest/registar.h>


using namespace NAlice;
using namespace NAlice::NCuttlefish::NAppHostServices;

Y_UNIT_TEST_SUITE(Utils) {
    Y_UNIT_TEST(SetSetraceLabelForTextInput) {
        auto req = MakeIntrusive<TSubrequest>();
        TSpeechKitRequestProto skr;
        skr.MutableRequest()->MutableEvent()->SetText("some text");
        skr.MutableRequest()->MutableEvent()->SetType(EEventType::text_input);
        req->Request = skr;
        SetSetraceLabel(req);
        UNIT_ASSERT_STRINGS_EQUAL(req->SetraceLabel, "megamind/http text_input: some text");
    }
    Y_UNIT_TEST(SetSetraceLabelForVoiceInput) {
        auto req = MakeIntrusive<TSubrequest>();
        TSpeechKitRequestProto skr;
        auto asr = skr.MutableRequest()->MutableEvent()->AddAsrResult();
        auto fst = asr->AddWords();
        fst->SetValue("some");
        auto snd = asr->AddWords();
        snd->SetValue("text");
        skr.MutableRequest()->MutableEvent()->SetType(EEventType::voice_input);
        req->Request = skr;
        SetSetraceLabel(req);
        UNIT_ASSERT_STRINGS_EQUAL(req->SetraceLabel, "megamind/http voice_input: some text");
    }
    Y_UNIT_TEST(SetSetraceLabelForVoiceInputPredefinedWords) {
        auto req = MakeIntrusive<TSubrequest>();
        SetSetraceLabel(req, "some text");
        UNIT_ASSERT_STRINGS_EQUAL(req->SetraceLabel, "megamind/http voice_input: some text");
    }
}

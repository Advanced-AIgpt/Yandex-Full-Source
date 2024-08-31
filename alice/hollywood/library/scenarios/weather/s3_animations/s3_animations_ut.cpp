#include <alice/hollywood/library/s3_animations/s3_animations.h>
#include <alice/hollywood/library/scenarios/weather/s3_animations/s3_animations.h>

#include <alice/library/json/json.h>
#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/protos/endpoint/capability.pb.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywood::NWeather::NS3Animations {

namespace {

constexpr TStringBuf DIRECTIVE_JSON = R"({
 "draw_animation_directive": {
  "animations": [
   {
    "s3_directory": {
     "bucket": "https://quasar.s3.yandex.net",
     "path": "animations/weather/rain"
    }
   }
  ],
  "speaking_animation_policy": "PlaySpeakingEndOfTts",
  "name": "draw_animation_directive"
 }
}
)";

} // namespace

Y_UNIT_TEST_SUITE(S3AnimationsSuite) {
    Y_UNIT_TEST(TestRareCondition) {
        UNIT_ASSERT(TryGetS3AnimationPathFromCondition("volcanic-ash").Empty());
    }

    Y_UNIT_TEST(TestDirectiveJson) {
        auto path = TryGetS3AnimationPathFromCondition("thunderstorm");
        UNIT_ASSERT(path.Defined());
        UNIT_ASSERT_VALUES_EQUAL(*path, "animations/weather/rain");

        auto directive = BuildDrawAnimationDirective(*path, TAnimationCapability_TDrawAnimationDirective_ESpeakingAnimationPolicy_PlaySpeakingEndOfTts);

        google::protobuf::util::JsonOptions options;
        options.add_whitespace = true;
        UNIT_ASSERT_VALUES_EQUAL(JsonStringFromProto(directive, options), DIRECTIVE_JSON);
    }
}

} // namespace NAlice::NHollywood::NWeather::NS3Animations

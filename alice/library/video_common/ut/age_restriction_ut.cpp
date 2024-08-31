#include <alice/library/video_common/age_restriction.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NVideoCommon {

namespace {

bool AreAgeRestrictionsPassed(ui32 minAge, const TString& genre, EContentRestrictionLevel mode, bool isPornoQuery,
                             bool isFromGallery, bool isPlayerContinue, bool isVideoByDescriptor)
{
    TAgeRestrictionCheckerParams params;
    params.IsFromGallery = isFromGallery;
    params.IsPlayerContinue = isPlayerContinue;
    params.IsPornoQuery = isPornoQuery;
    params.IsPornoGenre = IsPornoGenre(genre);
    params.MinAge = minAge;
    params.RestrictionLevel = mode;
    params.IsVideoByDescriptor = isVideoByDescriptor;

    return PassesAgeRestriction(params);
}

Y_UNIT_TEST_SUITE(AgeRestrictionsTestSuite) {
    Y_UNIT_TEST(TestIsPornoGenre) {
        UNIT_ASSERT_VALUES_EQUAL(IsPornoGenre("porno"), true);
        UNIT_ASSERT_VALUES_EQUAL(IsPornoGenre("драма, эротика"), true);
        UNIT_ASSERT_VALUES_EQUAL(IsPornoGenre("драма, комедия, криминал, биография"), false);
        UNIT_ASSERT_VALUES_EQUAL(IsPornoGenre("мультфильм, драма, комедия, приключения"), false);
    }

    Y_UNIT_TEST(TestPassesAgeRestriction) {
        { // Check that 16+ content isn't available for children.
            AreAgeRestrictionsPassed(/* minAge= */ 16, /* genre= */ "", EContentRestrictionLevel::Children,
                                     /* isPornoQuery= */ false, /* isFromGallery= */ false,
                                     /* isPlayerContinue= */ false, /*isVideoByDescriptor*/ false);
        }
        { // Check that 16+ content is available for medium settings.
            AreAgeRestrictionsPassed(/* minAge= */ 16, /* genre= */ "", EContentRestrictionLevel::Medium,
                                     /* isPornoQuery = */ false, /* isFromGallery = */ false,
                                     /* isPlayerContinue = */ false, /*isVideoByDescriptor*/ false);
        }
        { // Check that 14+ content is available for children.
            AreAgeRestrictionsPassed(/* minAge= */ 14, /* genre= */ "", EContentRestrictionLevel::Children,
                                     /* isPornoQuery = */ false, /* isFromGallery = */ false,
                                     /* isPlayerContinue = */ false, /*isVideoByDescriptor*/ false);
        }
        { // Check that porn is not available for children, independently of item's min_age.
            AreAgeRestrictionsPassed(/* minAge= */ 14, /* genre= */ "porn", EContentRestrictionLevel::Children,
                                     /* isPornoQuery = */ false, /* isFromGallery = */ false,
                                     /* isPlayerContinue = */ false, /*isVideoByDescriptor*/ false);
        }
        { // Check that porn is not available for children, independently of item's min_age, even by a direct request.
            AreAgeRestrictionsPassed(/* minAge= */ 14, /* genre= */ "porn", EContentRestrictionLevel::Children,
                                     /* isPornoQuery = */ true, /* isFromGallery = */ false,
                                     /* isPlayerContinue = */ false, /*isVideoByDescriptor*/ false);
        }
        { // Check that porn is available in family mode by direct request.
            AreAgeRestrictionsPassed(/* minAge= */ 14, /* genre= */ "porn", EContentRestrictionLevel::Medium,
                                     /* isPornoQuery = */ true, /* isFromGallery = */ false,
                                     /* isPlayerContinue = */ false, /*isVideoByDescriptor*/ false);
        }
        { // Check that porn is available in family mode if we continue watching.
            AreAgeRestrictionsPassed(/* minAge= */ 14, /* genre= */ "porn", EContentRestrictionLevel::Medium,
                                     /* isPornoQuery = */ false, /* isFromGallery = */ false,
                                     /* isPlayerContinue = */ true, /*isVideoByDescriptor*/ false);
        }
        { // Check that 16+ content is available for children if video is by descriptor.
            AreAgeRestrictionsPassed(/* minAge= */ 16, /* genre= */ "", EContentRestrictionLevel::Children,
                                     /* isPornoQuery= */ false, /* isFromGallery= */ false,
                                     /* isPlayerContinue= */ false, /*isVideoByDescriptor*/ true);
        }
        { // Check that porn is available for children if video is by descriptor.
            AreAgeRestrictionsPassed(/* minAge= */ 16, /* genre= */ "porn", EContentRestrictionLevel::Children,
                                     /* isPornoQuery = */ false, /* isFromGallery = */ false,
                                     /* isPlayerContinue = */ false, /*isVideoByDescriptor*/ true);
        }
    }
}

} // namespace

}

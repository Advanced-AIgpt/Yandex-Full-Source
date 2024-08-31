#include <alice/cuttlefish/library/convert/methods.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/env.h>


using namespace NAlice::NCuttlefish::NConvert::NMethods;



Y_UNIT_TEST_SUITE(TemplatesTests) {

struct TFakeMessage { } FakeMessage;


// ------------------------------------------------------------------------------------------------
namespace {
    struct TCustomHandler1 {
        static bool IsSerializationNeeded(const TFakeMessage&) {
            return false;
        }
    };

    struct TCustomHandler2 { };
}  // anonymous namespace

Y_UNIT_TEST(IsSerializationNeededForCustomHandlers) {
    UNIT_ASSERT(!TCustom<TCustomHandler1>::IsSerializationNeeded(FakeMessage));
    UNIT_ASSERT(TCustom<TCustomHandler2>::IsSerializationNeeded(FakeMessage));
}


// ------------------------------------------------------------------------------------------------
namespace {
    struct TAllocatedCustomHandler1 {
        bool IsSerializationNeeded(const TFakeMessage&) const {
            return false;
        }
    } const AllocatedCustomHandler1;

    struct TAllocatedCustomHandler2 { } const AllocatedCustomHandler2;
}  // anonymous namespace


Y_UNIT_TEST(IsSerializationNeededForAllocatedCustomHandlers) {
    UNIT_ASSERT(!TCustomAllocated<AllocatedCustomHandler1>::IsSerializationNeeded(FakeMessage));
    UNIT_ASSERT(TCustomAllocated<AllocatedCustomHandler2>::IsSerializationNeeded(FakeMessage));
}

}  // Y_UNIT_TEST_SUITE(ParseSynchronizeState)

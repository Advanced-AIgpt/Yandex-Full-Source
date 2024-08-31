#include <alice/megamind/library/protos/all_typed_callbacks.pb.h>

#include <alice/protos/extensions/extensions.pb.h>

#include <google/protobuf/descriptor.pb.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice::NMegamind;

namespace {

Y_UNIT_TEST_SUITE(TestAllTypedCallbacks) {
    Y_UNIT_TEST(TestAllCallbacksHasOption) {
        THashSet<TString> allCallbackNames;
        TTypedCallback allCallbacks;
        const auto* descriptor = allCallbacks.GetDescriptor();
        const auto* callbacksOneofDescriptor = descriptor->FindOneofByName("Callback");
        UNIT_ASSERT_C(callbacksOneofDescriptor, "No Callback oneof field in TTypedCallback proto");
        for (auto idx = 0; idx < callbacksOneofDescriptor->field_count(); ++idx) {
            const auto* callbackDescriptor = callbacksOneofDescriptor->field(idx)->message_type();
            UNIT_ASSERT_C(callbackDescriptor->options().HasExtension(NAlice::MMTypedCallbackName),
                          "Typed callback " + callbackDescriptor->name() + " should contains MMTypedCallbackName option with unique value");
            const auto callbackName = callbackDescriptor->options().GetExtension(NAlice::MMTypedCallbackName);
            UNIT_ASSERT_C(!allCallbackNames.contains(callbackName),
                          "Only one callback can use " + callbackName + " value as MMTypedCallbackName option");
            allCallbackNames.insert(callbackName);
        }
    }
}

} // namespace

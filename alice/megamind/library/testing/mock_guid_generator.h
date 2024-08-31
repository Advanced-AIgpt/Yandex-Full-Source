#pragma once

#include <alice/megamind/library/response/builder.h>

#include <library/cpp/testing/gmock_in_unittest/gmock.h>

namespace NAlice::NMegamind {

class TMockGuidGenerator : public IGuidGenerator {
public:
    MOCK_METHOD(TString, GenerateGuid, (), (const, override));
    MOCK_METHOD(TIntrusivePtr<IGuidGenerator>, Clone, (), (const, override));
};

} // namespace NAlice::NMegamind

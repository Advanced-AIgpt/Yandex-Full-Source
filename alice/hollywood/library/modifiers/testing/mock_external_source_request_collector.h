#pragma once

#include <alice/hollywood/library/modifiers/external_sources/request_collector.h>
#include <library/cpp/testing/gmock_in_unittest/gmock.h>

namespace NAlice::NHollywood::NModifiers {

class TMockExternalSourceRequestCollector : public IExternalSourceRequestCollector {
public:
    MOCK_METHOD(void, AddRequest, (const google::protobuf::Message& item, const TStringBuf type), (override));
};

} // namespace NAlice::NHollywood

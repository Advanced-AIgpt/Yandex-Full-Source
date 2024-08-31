#include "mock_global_context.h"

#include <alice/library/logger/logger.h>

#include <library/cpp/resource/resource.h>
#include <library/cpp/testing/unittest/tests_data.h>

#include <util/generic/singleton.h>

namespace NAlice {
namespace {

template <typename T>
const T& PatchSources(T& sources, const TString& url) {
    const auto* descriptor = sources.GetDescriptor();
    const auto* reflection = sources.GetReflection();

    for (int i = 0, total = descriptor->field_count(); i < total; ++i) {
        auto* sourceProto = dynamic_cast<TConfig::TSource*>(
            reflection->MutableMessage(&sources, descriptor->field(i))
        );

        if (sourceProto) {
            sourceProto->SetUrl(url);
        }
    }

    return sources;
}

void FillSaasInfoInSourceConfig(TConfig::TSaasSourceOptions& config) {
    config.SetUnistatPrefix("recommender");
    config.SetServiceName("ServiceName");
    config.SetWordsCountMinInRequest(0);
    auto& params = *config.MutableSaasQueryParams();
    params.SetFormula("Fromula");
    params.SetKps("Kps");
    params.SetThreshold(0);
    params.SetSoftness("0");
}

} // namespace

NTesting::TGenericInitMockedGlobalCtx::TGenericInitMockedGlobalCtx() {
    FillSaasInfoInSourceConfig(*Config.MutableSaasSkillDiscoveryOptions());
}

const NTesting::TGenericInitMockedGlobalCtx& TMockGlobalContext::GenericInit() {
    using namespace testing;

    if (GenericInit_.Defined()) {
        return *GenericInit_;
    }

    GenericInit_.ConstructInPlace();

    ON_CALL(Const(*this), ProxyHost()).WillByDefault(ReturnRef(Default<TString>()));
    ON_CALL(Const(*this), ProxyPort()).WillByDefault(Return(0));
    ON_CALL(Const(*this), ProxyHeaders()).WillByDefault(ReturnRef(Default<THttpHeaders>()));

    EXPECT_CALL(*this, RTLogger).WillRepeatedly([](TStringBuf, bool, TMaybe<ELogPriority>) {
        return CreateNullLogger();
    });
    EXPECT_CALL(*this, BaseLogger()).WillRepeatedly(ReturnRef(TRTLogger::NullLogger()));
    EXPECT_CALL(Const(*this), Config()).WillRepeatedly(ReturnRef(GenericInit_->Config));
    EXPECT_CALL(Const(*this), RngSeedSalt()).WillRepeatedly(ReturnRef(GenericInit_->RngSalt));
    EXPECT_CALL(*this, ServiceSensors()).WillRepeatedly(ReturnRef(GenericInit_->Sensors));
    EXPECT_CALL(Const(*this), GetFactorDomain()).WillRepeatedly(ReturnRefOfCopy(NFactorSlices::TFactorDomain{}));

    return *GenericInit_;
}

} // namespace NAlice

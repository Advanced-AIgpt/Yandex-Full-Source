#pragma once

#include <alice/begemot/lib/feature_aggregator/proto/config.pb.h>

#include <util/generic/maybe.h>
#include <util/generic/string.h>

namespace NAlice::NFeatureAggregator {

TString GenerateProtoEnum(
    const TFeatureAggregatorConfig& config,
    TStringBuf enumName,
    TMaybe<TStringBuf> protoPackage = Nothing(),
    TMaybe<TStringBuf> goPackage = Nothing(),
    TMaybe<TStringBuf> javaPackage = Nothing()
);

} // namespace NAlice::NFeatureAggregator

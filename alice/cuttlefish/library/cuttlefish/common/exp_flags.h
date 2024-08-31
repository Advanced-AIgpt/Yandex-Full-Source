#pragma once
#include <alice/cuttlefish/library/protos/session.pb.h>
#include <alice/megamind/protos/common/experiments.pb.h>
#include <library/cpp/json/writer/json_value.h>
#include <util/generic/maybe.h>
#include <util/generic/string.h>


namespace NAlice::NCuttlefish::NExpFlags {

extern const TString DISREGARD_UAAS;
extern const TString ONLY_100_PERCENT_FLAGS;
extern const TString SEND_PROTOBUF_TO_MEGAMIND;

// return value of experiment "name=value"
TMaybe<TString> GetExperimentValue(const NAliceProtocol::TRequestContext& requestCtx, const TStringBuf name);

bool ConductingExperiment(const NAliceProtocol::TRequestContext& requestCtx, const TString& key);

bool ParseExperiments(const NJson::TJsonValue& jsonExperiments, NAlice::TExperimentsProto& protoExperiments);

bool ExperimentFlagHasTrueValue(const NAliceProtocol::TRequestContext& requestContext, const TString& exp);

} // namespace NAlice::NCuttlefish::NExpFlags

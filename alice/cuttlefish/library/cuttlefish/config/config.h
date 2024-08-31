#pragma once

#include <alice/cuttlefish/library/proto_configs/cuttlefish.cfgproto.pb.h>

#include <library/cpp/json/json_value.h>
#include <util/generic/set.h>
#include <util/generic/map.h>


namespace NAlice::NCuttlefish {

NAliceCuttlefishConfig::TConfig GetDefaultCuttlefishConfig();
NAliceCuttlefishConfig::TConfig GetCuttlefishConfigFromResource(const TString& resource);
NAliceCuttlefishConfig::TConfig ParseCuttlefishConfigFromArgs(int argc, const char** argv);

}  // namespace NAlice::NCuttlefish

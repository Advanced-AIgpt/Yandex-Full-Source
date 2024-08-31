#pragma once

#include <alice/megamind/protos/common/data_source_type.pb.h>

namespace NAlice::NScenarios {

inline constexpr TStringBuf DATASOURCE_PREFIX = "datasource_";

const TString& GetDataSourceContextName(EDataSourceType type);
EDataSourceType GetDataSourceContextType(const TString& name);

} // namespace NAlice::NScenarios

#include "data_sources.h"

#include <alice/megamind/protos/scenarios/request.pb.h>
#include <google/protobuf/descriptor.pb.h>

#include <util/generic/algorithm.h>
#include <util/generic/hash.h>
#include <util/string/builder.h>


namespace NAlice::NScenarios {

namespace {

THashMap<EDataSourceType, TString> CreateDataSourceNames() {
    NScenarios::TDataSource dataSource;
    const auto* oneOfDescriptor = dataSource.descriptor()->oneof_decl(0);

    THashMap<EDataSourceType, TString> dataSourceNames;
    dataSourceNames.reserve(oneOfDescriptor->field_count());
    for (int i = 0; i < oneOfDescriptor->field_count(); ++i) {
        const auto dataSourceType = oneOfDescriptor->field(i)->options().GetExtension(NScenarios::DataSourceType);
        dataSourceNames.emplace(dataSourceType, TString::Join(DATASOURCE_PREFIX, EDataSourceType_Name(dataSourceType)));
    }
    return dataSourceNames;
}

const THashMap<EDataSourceType, TString> DATASOURCE_NAMES = CreateDataSourceNames();

} // anonymous namespace

const TString& GetDataSourceContextName(EDataSourceType type) {
    if (const auto* ptr = DATASOURCE_NAMES.FindPtr(type); ptr != nullptr) {
        return *ptr;
    }
    return Default<TString>();
}

EDataSourceType GetDataSourceContextType(const TString& name) {
    TString nameWithPrefix = TString::Join(DATASOURCE_PREFIX, name);
    const auto ptr = FindIf(DATASOURCE_NAMES, [nameWithPrefix](const auto& ds) -> bool {
        return ds.second == nameWithPrefix;
    });
    return ptr ? ptr->first : EDataSourceType::UNDEFINED_DATA_SOURCE;
}

} // namespace NAlice::NScenarios

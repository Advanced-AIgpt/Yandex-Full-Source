#include "data_sources.h"

#include <library/cpp/testing/unittest/registar.h>

#include <alice/megamind/protos/scenarios/request.pb.h>
#include <google/protobuf/descriptor.pb.h>

#include <util/generic/hash.h>

using namespace NAlice;

namespace {

const THashSet<EDataSourceType> ABSTRACT_DATASOURCES = {
    EDataSourceType::UNDEFINED_DATA_SOURCE,
    EDataSourceType::EMPTY_DEVICE_STATE,
    EDataSourceType::BEGEMOT_BEGGINS_RESULT,
};

} // namespace

Y_UNIT_TEST_SUITE(DataSources) {
    Y_UNIT_TEST(DataSourceToTypeToName) {
        NScenarios::TDataSource dataSource;

        const auto* descriptor = dataSource.descriptor();

        UNIT_ASSERT_VALUES_UNEQUAL(descriptor, nullptr);
        UNIT_ASSERT_VALUES_EQUAL(descriptor->oneof_decl_count(), 1);

        const auto* oneOfDescriptor = descriptor->oneof_decl(0);
        UNIT_ASSERT_VALUES_UNEQUAL(oneOfDescriptor, nullptr);
        UNIT_ASSERT_VALUES_UNEQUAL(oneOfDescriptor->field_count(), 0);

        THashSet<EDataSourceType> dataSourceTypes;
        for (int i = 0; i < oneOfDescriptor->field_count(); ++i) {
            const auto* fieldDescriptor = oneOfDescriptor->field(i);
            UNIT_ASSERT_VALUES_UNEQUAL(fieldDescriptor, nullptr);
            const auto& options = fieldDescriptor->options();

            UNIT_ASSERT_VALUES_EQUAL_C(
                options.HasExtension(NScenarios::DataSourceType), true,
                TStringBuilder{} << "Field " << fieldDescriptor->full_name() << " is missing DataSourceType option"
            );
            const auto dataSourceType = options.GetExtension(NScenarios::DataSourceType);
            UNIT_ASSERT(!ABSTRACT_DATASOURCES.find(dataSourceType));
            dataSourceTypes.insert(dataSourceType);
        }
        UNIT_ASSERT_VALUES_EQUAL(dataSourceTypes.size(), oneOfDescriptor->field_count());

        const auto* enumDescriptor = EDataSourceType_descriptor();
        UNIT_ASSERT_VALUES_UNEQUAL(enumDescriptor, nullptr);

        UNIT_ASSERT_VALUES_EQUAL(enumDescriptor->value_count() - ABSTRACT_DATASOURCES.size(), dataSourceTypes.size());
    }
}

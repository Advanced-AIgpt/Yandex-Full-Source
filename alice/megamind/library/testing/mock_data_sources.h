#pragma once

#include <alice/megamind/library/scenarios/interface/data_sources.h>

#include <library/cpp/testing/gmock_in_unittest/gmock.h>


namespace NAlice::NMegamind {

class TMockDataSources : public NMegamind::IDataSources {
public:
    MOCK_METHOD(const NScenarios::TDataSource&, GetDataSource, (EDataSourceType type), (override));
};

}

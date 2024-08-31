#include "memento.h"

#include <alice/library/unittest/message_diff.h>

#include <library/cpp/testing/unittest/registar.h>

#include <google/protobuf/wrappers.pb.h>

using namespace NAlice::NMegamind;

namespace {

Y_UNIT_TEST_SUITE(TestMemento) {
    Y_UNIT_TEST(TestDeserialization) {
        const TString serialized{"CgASKAokYTEzYTE0ZGEtMDQ4Zi00NzU0LWE0MzQtZjI3N2IwOTg4ZjZhEgAaagoETU1HTxJiCjB0eXBlLmdvb"
                                 "2dsZWFwaXMuY29tL05FeGFtcGxlQXBwcy5UTWVtZW50b1BheWxvYWQSLgokNGY3MmY2YmUtZjllMi00NTM4LT"
                                 "k5ZDItY2U0NWZjZWFlMmU2EIDa2/0FGAE="};
        auto data = DeserializeMementoData(serialized);
        UNIT_ASSERT_C(data.GetScenarioData("MMGO"), "There should be MMGO scenario");
    }

    Y_UNIT_TEST(TestMementoDataView) {
        NMementoApi::TRespGetAllObjects getAllObjectsResponse;
        auto& scenarioData = *getAllObjectsResponse.MutableScenarioData();

        const TString scenarioName{"myScenario"};
        const TString deviceId{"myDeviceId"};
        const TString uuid{"myUuid"};

        auto& surfaceScenarioDataMap = *getAllObjectsResponse.MutableSurfaceScenarioData();
        auto& surfaceScenarioDataDeviceId = *surfaceScenarioDataMap[deviceId].MutableScenarioData();
        auto& surfaceScenarioDataUuid = *surfaceScenarioDataMap[uuid].MutableScenarioData();

        google::protobuf::StringValue expectedValue;
        expectedValue.set_value("expectedValue");
        scenarioData[scenarioName].PackFrom(expectedValue);
        surfaceScenarioDataDeviceId[scenarioName].PackFrom(expectedValue);
        surfaceScenarioDataUuid[scenarioName].PackFrom(expectedValue);
        google::protobuf::Int32Value otherValue;
        otherValue.set_value(42);
        scenarioData["otherScenario"].PackFrom(otherValue);
        surfaceScenarioDataDeviceId["otherScenario"].PackFrom(otherValue);
        surfaceScenarioDataUuid["otherScenario"].PackFrom(otherValue);



        NMementoApi::TUserConfigs userConfigs;
        userConfigs.MutableConfigForTests()->SetDefaultSource("source");
        userConfigs.MutableNewConfig()->SetDefaultSource("another source");
        getAllObjectsResponse.MutableUserConfigs()->CopyFrom(userConfigs);

        TMementoData mementoData{getAllObjectsResponse};

        /* testScenarioData */ {
            TMementoDataView mementoDataView{mementoData, scenarioName, Default<TString>(), Default<TString>()};
            google::protobuf::StringValue actualValue;
            const auto* actualScenarioData = mementoDataView.GetScenarioData();
            UNIT_ASSERT(actualScenarioData);
            actualScenarioData->UnpackTo(&actualValue);
            UNIT_ASSERT_MESSAGES_EQUAL(expectedValue, actualValue);
            UNIT_ASSERT_MESSAGES_EQUAL(userConfigs, mementoDataView.GetUserConfigs());
        }
        /* testSurfaceScenarioDataByDeviceId */ {
            TMementoDataView mementoDataView{mementoData, scenarioName, deviceId, Default<TString>()};
            google::protobuf::StringValue actualValue;
            const auto* actualScenarioData = mementoDataView.GetSurfaceScenarioData();
            UNIT_ASSERT(actualScenarioData);
            actualScenarioData->UnpackTo(&actualValue);
            UNIT_ASSERT_MESSAGES_EQUAL(expectedValue, actualValue);
            UNIT_ASSERT_MESSAGES_EQUAL(userConfigs, mementoDataView.GetUserConfigs());
        }
        /* testSurfaceScenarioDataByUuid */ {
            TMementoDataView mementoDataView{mementoData, scenarioName, Default<TString>(), uuid};
            google::protobuf::StringValue actualValue;
            const auto* actualScenarioData = mementoDataView.GetSurfaceScenarioData();
            UNIT_ASSERT(actualScenarioData);
            actualScenarioData->UnpackTo(&actualValue);
            UNIT_ASSERT_MESSAGES_EQUAL(expectedValue, actualValue);
            UNIT_ASSERT_MESSAGES_EQUAL(userConfigs, mementoDataView.GetUserConfigs());
        }
    }
}

} // namespace

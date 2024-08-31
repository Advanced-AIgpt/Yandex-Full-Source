#include "fast_data.h"

#include <alice/hollywood/library/fast_data/ut/proto/test_fast_data.pb.h>

#include <alice/library/unittest/message_diff.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/stream/file.h>

#include <memory>

namespace NAlice::NHollywood {

class TTestFastData : public IFastData {
public:
    TTestFastData(const TTestFastDataProto& proto)
        : TestString_(proto.GetTestString())
    {
    }

    const TString& GetString() const {
        return TestString_;
    }

private:
    TString TestString_;
};

TTestFastDataProto RegisterAndCreateAndDumpTestFastData(TFastData& fastData) {
    static const TString fileName = "test_proto.pb";

    std::pair<TFastDataProtoProducer, TFastDataProducer> producers{
        []() { return std::make_shared<TTestFastDataProto>(); },
        [](TScenarioFastDataProtoPtr proto) {
            return std::make_shared<TTestFastData>(dynamic_cast<const TTestFastDataProto&>(*proto));
        }
    };
    fastData.Register({{fileName, producers}});

    TTestFastDataProto testFastDataProto;
    testFastDataProto.SetTestString("blablabla");

    TFileOutput fout(fileName);
    testFastDataProto.SerializeToArcadiaStream(&fout);

    return testFastDataProto;
}

void CreateSVNInfoFile() {
    TFileOutput fout(".svninfo");
    fout << "fast_data_version: 1";
}

Y_UNIT_TEST_SUITE(FastData) {
    Y_UNIT_TEST(Smoke) {
        CreateSVNInfoFile();

        TFastData fastData(".");
        auto testFastDataProto = RegisterAndCreateAndDumpTestFastData(fastData);

        fastData.Reload();

        const auto& retrievedFastData = *fastData.GetFastData<TTestFastData>();

        UNIT_ASSERT_EQUAL(testFastDataProto.GetTestString(), retrievedFastData.GetString());
    }

    Y_UNIT_TEST(MissingDataDir) {
        CreateSVNInfoFile();

        // Empty path, FastData should allways return empty protobuf
        TFastData fastData("");

        auto testFastDataProto = RegisterAndCreateAndDumpTestFastData(fastData);
        testFastDataProto.Clear();

        fastData.Reload();

        const auto& retrievedFastData = *fastData.GetFastData<TTestFastData>();

        UNIT_ASSERT_EQUAL(testFastDataProto.GetTestString(), retrievedFastData.GetString());
    }
}


} // namespace NAlice::NHollywood

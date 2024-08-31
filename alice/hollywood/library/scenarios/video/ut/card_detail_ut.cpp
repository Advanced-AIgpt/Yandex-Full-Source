#include <alice/library/json/json.h>
#include <library/cpp/testing/unittest/registar.h>

#include <alice/protos/data/video/card_detail.pb.h>
#include <util/stream/file.h>

namespace NAlice::NHollywood::NVideo {

Y_UNIT_TEST_SUITE(CardDetail) {
    Y_UNIT_TEST(MakeCardDetailResponse) {
        TFileInput inputData("fixtures/card_detail_ww_false.json");
        
        NJson::TJsonValue jsonResponse = JsonFromString(inputData.ReadAll());
        UNIT_ASSERT(jsonResponse.Has("will_watch") && jsonResponse["will_watch"] == false);
        {
            const TTvCardDetailResponse protoResponse = JsonToProto<TTvCardDetailResponse>(jsonResponse, true, true);
            UNIT_ASSERT(protoResponse.GetWillWatch() == false);
        }

        jsonResponse.InsertValue("will_watch", true);
        UNIT_ASSERT(jsonResponse["will_watch"] == true);
        {
            const TTvCardDetailResponse protoResponse = JsonToProto<TTvCardDetailResponse>(jsonResponse, true, true);
            UNIT_ASSERT(protoResponse.GetWillWatch() == true);
        }
    }
}
}


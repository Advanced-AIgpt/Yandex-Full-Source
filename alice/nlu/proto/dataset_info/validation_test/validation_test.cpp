#include <alice/library/json/json.h>
#include <alice/nlu/proto/dataset_info/dataset_info.pb.h>
#include <alice/protos/data/language/language.pb.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/string.h>
#include <util/stream/file.h>

namespace {

void ValidateLanguage(const NAlice::NNlu::TIntentDatasets& intentDatasets, const NAlice::ELang language) {
    for (auto&& [intent, datasets] : intentDatasets.GetIntentDatasets()) {
        for (auto&& dataset : datasets.GetDatasetInfos()) {
            UNIT_ASSERT(dataset.GetLanguage() == language);
        }
    }
}

void ValidateRequiredFields(const NAlice::NNlu::TIntentDatasets& intentDatasets) {
    for (auto&& [intent, datasets] : intentDatasets.GetIntentDatasets()) {
        for (auto&& dataset : datasets.GetDatasetInfos()) {
            UNIT_ASSERT(dataset.HasAcceptDataTable() || dataset.HasDevDataTable() || dataset.HasKpiDataTable());
            UNIT_ASSERT(!dataset.HasAcceptDataTable() || dataset.GetAcceptDataTable());
            UNIT_ASSERT(!dataset.HasDevDataTable() || dataset.GetDevDataTable());
            UNIT_ASSERT(!dataset.HasKpiDataTable() || dataset.GetKpiDataTable());

            if (dataset.HasSlotMarkupInfo()) {
                UNIT_ASSERT(dataset.GetSlotMarkupInfo().GetTable());
                UNIT_ASSERT_GT(dataset.GetSlotMarkupInfo().GetSlots().size(), 0);
            }

            if (dataset.HasVoiceRecordingsInfo()) {
                UNIT_ASSERT(dataset.GetVoiceRecordingsInfo().GetTable());
            }
        }
    }
}


void Validate(const NAlice::NNlu::TIntentDatasets& intentDatasets, const NAlice::ELang language) {
    ValidateLanguage(intentDatasets, language);
    ValidateRequiredFields(intentDatasets);
}

} // namespace

Y_UNIT_TEST_SUITE(TestValidity) {
    Y_UNIT_TEST(TestArabic) {
        TFileInput inputJsonStream = TFileInput(BinaryPath("alice/nlu/data/ar/datasets/data.json"));
        const NJson::TJsonValue jsonConfig = NJson::ReadJsonTree(&inputJsonStream, /* throwOnError */ true);
        const NAlice::NNlu::TIntentDatasets intentDatasets = NAlice::JsonToProto<NAlice::NNlu::TIntentDatasets>(jsonConfig);

        Validate(intentDatasets, NAlice::L_ARA);
    }
}

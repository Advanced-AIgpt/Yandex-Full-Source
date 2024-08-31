#include "query_wizard_features_reader_wrapper.h"

#include <util/generic/singleton.h>
#include <alice/nlu/query_wizard_features/reader/reader.h>

#define READER_PTR(x) ((NQueryWizardFeatures::TReader*)(x))


struct TErrorMessageHolder {
    TString Message;
};

extern "C" {
EXPORT ReaderHandle* ReaderCreate() {
    try {
        return new NQueryWizardFeatures::TReader;
    } catch (...) {
        Singleton<TErrorMessageHolder>()->Message = CurrentExceptionMessage();
    }

    return nullptr;
}

EXPORT const char* GetErrorString() {
    return Singleton<TErrorMessageHolder>()->Message.data();
}

EXPORT void ReaderDelete(ReaderHandle* reader) {
    if (reader != nullptr) {
        delete READER_PTR(reader);
    }
}

EXPORT bool LoadTrie(
    ReaderHandle* reader,
    const char* triePath,
    const char* dataPath) {
    try {
        READER_PTR(reader)->Load(triePath, dataPath);
    } catch (...) {
        Singleton<TErrorMessageHolder>()->Message = CurrentExceptionMessage();
        return false;
    }

    return true;
}

EXPORT bool GetFeaturesForTextFragments(ReaderHandle* reader, const char *text, TVector<TFragmentFeatures>* result) {
    try {
        const auto allFragmentFeatures = READER_PTR(reader)->GetFeaturesForTextFragments(text);

        for (const auto& fragmentFeatures : allFragmentFeatures) {
            TFragmentFeatures fragment;

            fragment.Fragment = fragmentFeatures.Fragment;
            fragment.NormalizedFragment = fragmentFeatures.NormalizedFragment;
            fragment.Start = fragmentFeatures.CharStart;
            fragment.Length = GetNumberOfUTF8Chars(fragmentFeatures.Fragment);

            const auto& features = fragmentFeatures.Features;

            auto addWizardFeatures = [&fragment](const NQueryWizardFeatures::TWizardData& wizardData) {
                fragment.Features.push_back(wizardData.GetCountPerRequest());
                fragment.Features.push_back(wizardData.GetSurplus());
            };

            fragment.Features.push_back(features.GetUsersPerDay());
            addWizardFeatures(features.GetImages());
            addWizardFeatures(features.GetVideo());
            addWizardFeatures(features.GetWeather());
            addWizardFeatures(features.GetMusic());
            addWizardFeatures(features.GetEntity());
            addWizardFeatures(features.GetCompanies());
            addWizardFeatures(features.GetMaps());
            addWizardFeatures(features.GetBno());
            addWizardFeatures(features.GetMarket());

            result->push_back(fragment);
        }
    } catch (...) {
        Singleton<TErrorMessageHolder>()->Message = CurrentExceptionMessage();
        return false;
    }
    return true;
}

}

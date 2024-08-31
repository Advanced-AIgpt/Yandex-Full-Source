#include "decoder.h"

#include <alice/bitbucket/pynorm/util/util/mem_blob.h>

#include <util/charset/unidata.h>
#include <util/charset/utf8.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/memory/blob.h>
#include <util/stream/input.h>

namespace NAlice {

    mem_blob_t ToMemBlob(const TBlob& blob) {
        return {const_cast<void*>(blob.Data()), blob.Length()};
    }

    TFstDecoder::TFstDecoder(const TString& normalizerDataPath) {
        NormalizerData.Reset(norm_data_read(normalizerDataPath.c_str()));
        Y_ENSURE(NormalizerData, "Failed to load the normalizer data");
    }

    TFstDecoder::TFstDecoder(const IDataLoader& loader) {
        const auto flags = loader.LoadBlob("flags.txt");
        const auto symbols = loader.LoadBlob("symbols.sym");
        TVector<fst_data_t> fsts;
        TString fstName;
        auto sequence = loader.GetInputStream("sequence.txt");
        TVector<TString> fstNames;
        TVector<TBlob> blobs;
        while (sequence->ReadLine(fstName)) {
            fst_data_t fstData;
            fstNames.emplace_back(std::move(fstName));
            fstData.fst_name = fstNames.back().c_str();
            blobs.push_back(loader.LoadBlob(fstNames.back() + ".fst"));
            fstData.blob = ToMemBlob(blobs.back());
            fsts.push_back(fstData);
        }
        NormalizerData.Reset(norm_data_read_from_blobs(
            ToMemBlob(flags),
            ToMemBlob(symbols),
            fsts.data(),
            fsts.size()));
        Y_ENSURE(NormalizerData, "Failed to load the normalizer data");
    }

    TFstDecoder::TFstDecoder(const IDataLoader& loader, TVector<TString> blackList)
        : TFstDecoder(loader)
    {
        BlackList = std::move(blackList);
        BlackListConverted.reserve(BlackList.size() + 1u);
        for (auto& str : BlackList) {
            BlackListConverted.emplace_back(str.data());
        }
        BlackListConverted.emplace_back(nullptr);
    }

    TString TFstDecoder::Normalize(const TString& text) const {
        char** blackList = BlackListConverted.empty() ? nullptr : const_cast<char**>(BlackListConverted.data());
        THolder<const char, TFree> normalized  = THolder<const char, TFree>(norm_run_with_blacklist(
            NormalizerData.Get()
            , const_cast<char*>(text.c_str())
            , blackList));
        if (!normalized) {
            return {};
        }
        TString result{normalized.Get()};
        return result;
    }

} // namespace NAlice

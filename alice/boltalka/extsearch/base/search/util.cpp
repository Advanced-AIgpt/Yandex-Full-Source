#include "util.h"

#include <util/stream/file.h>
#include <util/string/split.h>

namespace NNlg {

namespace {

TVector<ui64> LoadOffsets(const TString& offsetPath) {
    const TString offsetString = TFileInput(offsetPath).ReadAll();
    return {reinterpret_cast<const ui64*>(offsetString.data()),
            reinterpret_cast<const ui64*>(offsetString.data() + offsetString.size())};
}

}

TBlob GetBlobFromFile(const TString& path, EMemoryMode memoryMode) {
    if (memoryMode == EMemoryMode::Locked) {
        return TBlob::LockedFromFile(path);
    }
    return TBlob::PrechargedFromFile(path);
}

TBlobWithOffsets LoadBlobWithOffsets(const TString& fileName, EMemoryMode memoryMode) {
    auto offsets = LoadOffsets(fileName + "_offsets");
    auto data = GetBlobFromFile(fileName, memoryMode);
    offsets.push_back(data.Length());
    return {data, offsets};
}

TString DropEntityIdFromIndexName(const TString& fullIndexName) {
    TVector<TString> tokens = StringSplitter(fullIndexName).Split(':');
    return tokens[0];
}

bool IsEntityIndex(const TString& indexName) {
    return indexName != DropEntityIdFromIndexName(indexName);
}

}

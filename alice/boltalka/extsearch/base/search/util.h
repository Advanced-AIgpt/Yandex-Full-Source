#pragma once

#include <alice/boltalka/extsearch/base/util/memory_mode.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/memory/blob.h>
#include <util/system/defaults.h>

namespace NNlg {

struct TBlobWithOffsets {
    TBlob Data;
    TVector<ui64> Offsets;
};

TBlob GetBlobFromFile(const TString& path, EMemoryMode memoryMode);
TBlobWithOffsets LoadBlobWithOffsets(const TString& fileName, EMemoryMode memoryMode);

TString DropEntityIdFromIndexName(const TString& fullIndexName);

bool IsEntityIndex(const TString& indexName);

}

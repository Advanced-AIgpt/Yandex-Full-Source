#pragma once

#include <dict/mt/libs/nn/ynmt/config_helper/model_reader.h>

#include <util/generic/vector.h>
#include <util/memory/blob.h>
#include <util/stream/file.h>

template <typename T>
THashSet<T> LoadFileToHashSet(TString path) {
    THashSet<T> result;
    TFileInput f(path);
    TString line;
    while (f.ReadLine(line)) {
        if (line == "") {
            continue;
        }
        result.insert(FromString<T>(line));
    }

    return result;
}

TVector<TVector<float>> LoadPtuneFromNpz(const NDict::NMT::NYNMT::TModelProtoWithMeta* file);

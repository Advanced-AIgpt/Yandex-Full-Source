#pragma once

#include <alice/nlu/proto/entities/fst.pb.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NAlice::NNlu {

    bool IsArDualNum(TStringBuf token);
    TFstResult GetArDualNumEntities(const TVector<TString>& tokens);
    
} // namespace NAlice::NNlu

#pragma once

#include "value.h"

#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NAlice::NNlg {

class TGlobals {
public:
    TValue ResolveLoad(TStringBuf name, TStringBuf module = {}) const;
    TValue& ResolveStore(TStringBuf name);

    void RegisterImport(TStringBuf path, const TGlobals& globals);
    void RegisterFromImport(TStringBuf name, TStringBuf target, const TGlobals& globals);

    void Freeze();

private:
    THashMap<TString, TValue> Globals;

    // TODO(a-square): is one map better?
    THashMap<TString, const TGlobals*> Imports;
    THashMap<TString, std::pair<const TGlobals*, TString>> FromImports;
};

class TGlobalsChain {
public:
    TGlobalsChain(const TGlobalsChain* top, const TGlobals* bottom)
        : Top(top)
        , Bottom(bottom) {
    }

    TValue ResolveLoad(TStringBuf name, TStringBuf module = {}) const;

private:
    const TGlobalsChain* Top;
    const TGlobals* Bottom;
};

} // namespace NAlice::NNlg

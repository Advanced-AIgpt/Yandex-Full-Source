#pragma once

#include <library/cpp/langs/langs.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NAlice {

    namespace NFst {

        TString GetDataRoot();

        TString GetFstDataPath(const TStringBuf& fstName, ELanguage lang);

    } // namespace NFst

} // namespace NAlice

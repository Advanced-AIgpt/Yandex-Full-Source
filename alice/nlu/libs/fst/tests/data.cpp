#include "data.h"

#include <util/folder/path.h>

#include <library/cpp/testing/unittest/env.h>

namespace NAlice {

    namespace NFst {

        TString GetDataRoot() {
            return BuildRoot() + "/alice/begemot/lib/fst/data";
        }

        TString GetFstDataPath(const TStringBuf& fstName, ELanguage lang) {
            TFsPath path{GetDataRoot()};
            TString suffix = TString("fst/") + fstName;
            TString suffix2 = TString("fst/") + IsoNameByLanguage(lang) + "/" + fstName;
            path /= suffix;
            path /= suffix2; // suffix must be added twice

            return path;
        }

    } // namespace NFst

} // namespace NAlice

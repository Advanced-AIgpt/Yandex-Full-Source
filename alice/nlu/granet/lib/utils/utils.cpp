#include "utils.h"
#include <util/charset/wide.h>
#include <util/stream/file.h>
#include <util/stream/mem.h>
#include <util/stream/str.h>
#include <util/string/escape.h>

namespace NGranet {

// ~~~~ Other ~~~~

TVector<TString> LoadLines(const TFsPath& path) {
    TVector<TString> lines;
    TFileInput file(path);
    TString line;
    while (file.ReadLine(line)) {
        lines.push_back(line);
    }
    return lines;
}

} // namespace NGranet

#pragma once

#include <util/folder/iterator.h>
#include <util/folder/path.h>
#include <util/generic/vector.h>

namespace NAlice::NMegamind {

TDirIterator ListFolder(const TString& folderPath);

TVector<TFsPath> ListFilesInDirectory(const TString& folderPath);

} // namespace NAlice::NMegamind

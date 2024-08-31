#pragma once

#include <util/generic/vector.h>
#include <util/folder/path.h>

TVector<TString> LoadTextLinesFromDataset(const TFsPath& path);
void SaveTextLinesInTable(const TFsPath& path, const TVector<TString>& lines);

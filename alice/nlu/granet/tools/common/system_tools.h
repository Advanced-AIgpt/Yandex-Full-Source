#pragma once

#include <util/folder/path.h>

namespace NGranet {

TString RequireEnv(const TString& key);

TFsPath RequireArcadiaPath();

// Backup current Granet sources and results.
// Only if variables GRANET_BACKUP and ARCADIA are defined.
void Backup(const TVector<TFsPath>& items);

} // namespace NGranet

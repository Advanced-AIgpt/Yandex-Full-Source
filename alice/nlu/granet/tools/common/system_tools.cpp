#include "system_tools.h"
#include <util/datetime/cputimer.h>
#include <util/system/env.h>

namespace NGranet {

TString RequireEnv(const TString& key) {
    const TString value = GetEnv(key);
    Y_ENSURE(!value.empty(), "Environment variable " + value + " is not defined.");
    return value;
}

TFsPath RequireArcadiaPath() {
    return TFsPath(RequireEnv("ARCADIA"));
}

// Backup current Granet sources and results.
// Only if variables GRANET_BACKUP and ARCADIA are defined.
void Backup(const TVector<TFsPath>& items) {
    // Generate name and make directory for backup results and sources.
    // If variable GRANET_BACKUP is not defined, print message and returns undefined path.
    const TFsPath backupRoot = GetEnv("GRANET_BACKUP");
    if (!backupRoot.IsDefined()) {
        return;
    }
    const TFsPath backupDir = backupRoot / Now().FormatLocalTime("Backup_%Y_%m_%d_%H%M%S");
    backupDir.MkDirs();

    try {
        for (const TFsPath& path : items) {
            if (path.IsDefined()) {
                path.CopyTo(backupDir, true);
            }
        }
        const TFsPath sourcesDir = RequireArcadiaPath() / "alice/nlu/granet";
        sourcesDir.CopyTo(backupDir, true);
    } catch(const yexception& e) {
        Cerr << e.AsStrBuf() << Endl;
    }
}

} // namespace NGranet

#include "convert_music.h"
#include <alice/nlu/granet/tools/common/system_tools.h>
#include <library/cpp/getopt/last_getopt.h>

using namespace NGranet;

int main() {
    const TFsPath arcadiaDir = RequireArcadiaPath();
    const TFsPath resultsDir = arcadiaDir / "alice/nlu/granet/tools/convert_vins/results";

    TConvertMusicApplication(arcadiaDir, resultsDir).Process();

    Backup({resultsDir});

    return 0;
}

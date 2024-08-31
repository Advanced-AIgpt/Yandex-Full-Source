#include "utils.h"

#include <alice/megamind/library/config/config.h>

#include <library/cpp/testing/unittest/env.h>

#include <util/generic/string.h>

namespace NAlice {

TConfig CreateTestConfig(const ui16 port) {
    TConfig config{};
    config.MutableAppHost()->SetHttpPort(port);
    return config;
}

TConfig GetRealConfig() {
    const i32 argc = 5;
    const auto& configPath = TFsPath(ArcadiaSourceRoot()) / "alice/megamind/configs/production/megamind.pb.txt";
    const char* argv[argc] = {"", "-c", configPath.c_str(), "-p", "1"};
    return LoadConfig(argc, argv);
}

NMegamind::TClassificationConfig GetRealClassificationConfig() {
    return LoadClassificationConfig(TFsPath(ArcadiaSourceRoot()) / "alice/megamind/configs/common/classification.pb.txt");
}

} // namespace NAlice

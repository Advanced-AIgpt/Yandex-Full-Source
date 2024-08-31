#pragma once

namespace NAlice {
class TRTLogger;
}

namespace NAlice::NMegamind {

enum class EUniproxyStage {
    Run    /* "run" */,
    Apply  /* "apply" */,
};

enum class ERequestResult {
    Success /* "success" */,
    Fail    /* "fail" */,
};

} // namespace NAlice::NMegamind

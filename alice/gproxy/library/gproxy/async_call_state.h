#pragma once

namespace NGProxy {


enum class EAsyncCallState {
    Initializing,
    Executing,
    Waiting,
    Completing,
    Finished
};


}   // namespace NGProxy

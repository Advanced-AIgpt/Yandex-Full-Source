#include "worker.h"

namespace NAlice::NCuttlefish::NAppHostServices {

bool TMegamindServantWorker::PullQueue(TQueue<TMegamindServantAnswer>& queue) {
    if (AnswersQueue.empty()) {
        return false;
    }
    queue.swap(AnswersQueue);
    AnswersQueue.clear(); // maybe it's redundant?
    return true;
}

void TMegamindServantWorker::SetContexts(NAliceProtocol::TContextLoadResponse contexts) {
    Contexts = std::move(contexts);
}

void TMegamindServantWorker::FlushPartialsQueue() {
    // TODO(sparkle)
}

}  // namespace NAlice::NCuttlefish::NAppHostServices

#pragma once
#include <alice/cuttlefish/library/cuttlefish/megamind/client/client.h>
#include <alice/cuttlefish/library/protos/context_load.pb.h>
#include <contrib/libs/protobuf/src/google/protobuf/message.h>
#include <util/generic/maybe.h>
#include <util/generic/queue.h>


namespace NAlice::NCuttlefish::NAppHostServices {

struct TMegamindServantAnswer {
    const TStringBuf Type;
    std::unique_ptr<google::protobuf::Message> Item;
};

class TMegamindServantWorker {
public:
    // pulls out all items that we need to put into AppHost context, and clears the queue
    // returns true if the resulting queue is not empty
    bool PullQueue(TQueue<TMegamindServantAnswer>& queue);

public:
    void SetContexts(NAliceProtocol::TContextLoadResponse response);
    void FlushPartialsQueue();

public:
    // global static parameters
    bool IsPartialsEnabled() const { return false; } // TODO(sparkle)

    // message static parameters
    bool IsTextInput() const { return false; } // TODO(sparkle)
    bool IsVoiceInput() const { return false; } // TODO(sparkle)
    bool IsMusicInput() const { return false; } // TODO(sparkle)

    // dynamic parameters
    bool HasContexts() const { return Contexts.Defined(); }
    bool HasAsrEou() const { return false; } // TODO(sparkle)

private:
    TQueue<TMegamindServantAnswer> AnswersQueue;
    TMaybe<NAliceProtocol::TContextLoadResponse> Contexts;
    std::unique_ptr<IMegamindClient> Client;
};

}  // namespace NAlice::NCuttlefish::NAppHostServices

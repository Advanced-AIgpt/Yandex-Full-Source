#pragma once

#include <alice/cuttlefish/library/protos/context_load.pb.h>
#include <alice/cuttlefish/library/protos/music_match.pb.h>
#include <alice/cuttlefish/library/protos/session.pb.h>

namespace NAlice::NMusicMatch::NProtobuf {
    using TInitRequest = ::NMusicMatch::TInitRequest;
    using TInitResponse = ::NMusicMatch::TInitResponse;

    using TStreamRequest = ::NMusicMatch::TStreamRequest;
    using TStreamResponse = ::NMusicMatch::TStreamResponse;
    using TAddData = ::NMusicMatch::TAddData;
    using TMusicResult = ::NMusicMatch::TMusicResult;

    using TContextLoadResponse = ::NAliceProtocol::TContextLoadResponse;
    using TSessionContext = ::NAliceProtocol::TSessionContext;
}

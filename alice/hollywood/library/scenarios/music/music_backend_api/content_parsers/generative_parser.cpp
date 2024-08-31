#include "generative_parser.h"

namespace NAlice::NHollywood::NMusic {

void ParseGenerative(const NJson::TJsonValue& generativeJson, TMusicQueueWrapper& mq, bool hasMusicSubscription) {
    TQueueItem generativeStreamItem;
    generativeStreamItem.SetTrackId(generativeJson["stream"]["id"].GetStringSafe());
    generativeStreamItem.MutableGenerativeInfo()->SetGenerativeStationId(mq.ContentId().GetId());
    generativeStreamItem.MutableGenerativeInfo()->SetGenerativeStreamUrl(generativeJson["stream"]["url"].GetStringSafe());
    generativeStreamItem.SetTitle(generativeJson["data"]["title"].GetStringSafe());
    generativeStreamItem.SetCoverUrl(generativeJson["data"]["imageUrl"].GetStringSafe());
    generativeStreamItem.SetType("generative");
    generativeStreamItem.SetDurationMs(INT32_MAX);
    generativeStreamItem.SetContentWarning(EContentWarning::ChildSafe);
    mq.TryAddItem(std::move(generativeStreamItem), hasMusicSubscription);
}

} // NAlice::NHollywoood::NMusic

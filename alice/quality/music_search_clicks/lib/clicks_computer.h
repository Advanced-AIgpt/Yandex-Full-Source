#pragma once

#include <alice/quality/music_search_clicks/proto/io.pb.h>

#include <kernel/hosts/owner/owner.h>
#include <library/cpp/resource/resource.h>
#include <mapreduce/yt/interface/client.h>
#include <quality/personalization/big_rt/rapid_clicks_common/counters/calculator.h>
#include <quality/personalization/big_rt/rapid_clicks_common/proto/config.pb.h>
#include <quality/personalization/big_rt/rapid_clicks_common/proto/states.pb.h>

#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/memory/blob.h>

#include <type_traits>

namespace NAlice::NMusicWebSearch::NListeningCounters {

    // this namespace just for testing purposes
    namespace NImpl {
        NRapidClicks::TConfig CreateConfig();

        struct TCountersResult {
            TString Puid;
            TVector<std::pair<NRapidClicks::TKeys, NRapidClicks::TCounters>> Counters;
            THashMap<TString, ui64> Errors;
        };

        THashSet<TString> GeneratePossibleUrls(TStringBuf originalUrl);

        void AggregateListening(const TUserListening& listening, NRapidClicks::TCalculator& calculator);
        TCountersResult ExtractCounters(TStringBuf puid, NRapidClicks::TCalculator& calculator);

        template <typename Iterator>
        TCountersResult ComputeCounters(const NRapidClicks::TConfig& config, const Iterator begin, const Iterator end) {
            TString puid;
            NRapidClicks::TCalculator calculator(config, /* forceDeterministic */ true);

            for (Iterator it = begin; it != end; ++it) {
                const TUserListening* listening = nullptr;

                if constexpr (std::is_same<Iterator, NYT::TTableReaderIterator<TUserListening>>::value) {
                    listening = &((*it).GetRow());
                } else {
                    listening = it;
                }

                Y_ENSURE(listening);
                if (puid.Empty()) {
                    puid = listening->GetPuid();
                } else {
                    Y_ENSURE(puid == listening->GetPuid());
                }
                AggregateListening(*listening, calculator);
            }

            return ExtractCounters(puid, calculator);
        }

        struct TAggregationResult {
            NRapidClicks::TKeys MainKey;
            NRapidClicks::TAggregatorState FullState;
            THashMap<TString, ui64> Errors;
        };

        TAggregationResult AggregateCounters(const NRapidClicks::TConfig& config, const TCountersResult& countersResult);

    }  // namespace NImpl

    NYT::TTableSchema CreateCompactTableSchema();

    class TCollectCountersReducer: public NYT::IReducer<NYT::TTableReader<TUserListening>, NYT::TTableWriter<TFullState>> {
    public:
        TCollectCountersReducer()
            : Config(NImpl::CreateConfig())
        {
        }

        void Do(TReader* reader, TWriter* writer) override;

    private:
        NRapidClicks::TConfig Config;
    };

    class TCountersCompactifier: public NYT::IMapper<NYT::TTableReader<TFullState>, NYT::TTableWriter<TCompactState>> {
    public:
        TCountersCompactifier()
            : DomAttrCanonizer(TBlob::FromString(NResource::Find("/2ld.list")), TBlob::FromString(NResource::Find("/ungrouped.list")))
        {
        }

        void Do(TReader* reader, TWriter* writer) override;
        void PrepareOperation(const NYT::IOperationPreparationContext& context, NYT::TJobOperationPreparer& preparer) const override;
    private:
        TDomAttrCanonizer DomAttrCanonizer;
    };

}  // namespace NAlice::NMusicWebSearch::NListeningCounters

#include "clicks_computer.h"

#include <quality/personalization/big_rt/aggregator_vcdiff/lib/compactify_state.h>
#include <quality/personalization/big_rt/rapid_clicks_common/aggregator/mega_aggregator.h>
#include <quality/personalization/big_rt/rapid_clicks_common/config/search_middle.h>

#include <kernel/alice/music_scenario/web_url_canonizer/lib/web_url_canonizer.h>

#include <library/cpp/codecs/codecs.h>

#include <util/generic/algorithm.h>
#include <util/stream/output.h>

namespace NAlice::NMusicWebSearch::NListeningCounters {
    namespace {
        constexpr ::google::protobuf::uint64 MIDDLE_SEARCH_CONFIG_VERSION = 10;
        constexpr TStringBuf CODEC = "zstd_6";

        bool IsSupportedKey(const int key) {
            const auto enumKey = static_cast<NRapidClicks::EKeyType>(key);

            return enumKey == NRapidClicks::EKeyType::ICookie || enumKey == NRapidClicks::EKeyType::Url ||
                enumKey == NRapidClicks::EKeyType::UrlSearch || enumKey == NRapidClicks::EKeyType::StrongNormalizedUrl ||
                enumKey == NRapidClicks::EKeyType::Host;
        }

        const NRapidClicks::TKeys& FindMainKey(const NImpl::TCountersResult& countersResult) {
            for (const auto& [key, _] : countersResult.Counters) {
                if (key.KeysSize() == 1 && key.GetKeys(0).GetKeyType() == NRapidClicks::EKeyType::ICookie) {
                    return key;
                }
            }
            ythrow yexception() << "Single ICookie key not found";
        }

        IOutputStream& operator<< (IOutputStream& out, const THashMap<TString, ui64>& value) {
            for (const auto& [k, v] : value) {
                out << k << " : " << v << Endl;
            }

            return out;
        }
    } // namespace

    namespace NImpl {
        NRapidClicks::TConfig CreateConfig() {
            NRapidClicks::TConfig resultConfig;
            const NRapidClicks::TConfig baseConfig = NRapidClicks::CreateSearchMiddleConfig();

            for (const auto& keysConfig : baseConfig.GetKeysConfig()) {
                if (AllOf(keysConfig.GetTypes(), IsSupportedKey)) {
                    resultConfig.AddKeysConfig()->CopyFrom(keysConfig);
                }
            }
            for (const auto& countersConfig : baseConfig.GetCountersConfig()) {
                if (countersConfig.GetVersion() == MIDDLE_SEARCH_CONFIG_VERSION) {
                    resultConfig.AddCountersConfig()->CopyFrom(countersConfig);
                }
            }

            return resultConfig;
        }

        THashSet<TString> GeneratePossibleUrls(const TStringBuf originalUrl) {
            // this hackish function tries to generate all possible duplicate urls for given originalUrl

            THashSet<TString> possible;
            possible.emplace(originalUrl);

            const TStringBuf withoutCgi = originalUrl.Before('?');
            possible.emplace(withoutCgi);

            const TString canonized = NAlice::NMusic::CanonizeMusicUrl(originalUrl);
            Y_ENSURE(canonized, "can't canonize y.music url, canonization code could be broken");
            possible.insert(canonized);

            const NAlice::NMusic::TParseMusicUrlResult parseResult = NAlice::NMusic::ParseMusicDataFromDocUrl(originalUrl);

            if (parseResult.Type == NAlice::NMusic::EMusicUrlType::Artist) {
                TString withTracks = canonized + "/tracks";
                possible.emplace(std::move(withTracks));

                TString withAlbums = canonized + "/albums";
                possible.emplace(std::move(withAlbums));
            }

            return possible;
        }

        void AggregateListening(const TUserListening& listening, NRapidClicks::TCalculator& calculator) {
            for (const auto& url: GeneratePossibleUrls(listening.GetUrl())) {
                NRapidClicks::TCalculatorContext context;

                context.Dwelltime = static_cast<ui64>(listening.GetTimespent());
                context.Timestamp = listening.GetTimestamp();

                context.Uid = listening.GetPuid();
                context.CacheableUrl.Url = url;

                calculator.CalcCounters(context);
            }
        }

        TCountersResult ExtractCounters(const TStringBuf puid, NRapidClicks::TCalculator& calculator) {
            TVector<std::pair<TString, NRapidClicks::TCounters>> rawCounters = calculator.OutputCounters();
            TVector<std::pair<NRapidClicks::TKeys, NRapidClicks::TCounters>> counters(rawCounters.size());

            for (size_t i = 0; i < rawCounters.size(); ++i) {
                auto [rawKey, keyCounters] = rawCounters[i];

                NRapidClicks::TKeys key;
                Y_ENSURE(key.ParseFromString(rawKey));

                counters[i].first = std::move(key);
                counters[i].second = std::move(keyCounters);
            }

            return {.Puid = TString(puid), .Counters = counters, .Errors = calculator.GetAndDeleteLogs()};
        }

        TAggregationResult AggregateCounters(const NRapidClicks::TConfig& config, const TCountersResult& countersResult) {
            if (countersResult.Counters.empty()) {
                return {};
            }

            NRapidClicks::TMegaAggregator megaAggregator(config, /* forceDeterministic */ true);
            const NRapidClicks::TKeys mainKey = FindMainKey(countersResult);

            // initialize main key with fake state
            megaAggregator.ReInitializeFromState(mainKey, NRapidClicks::TAggregatorState());

            for (const auto& [key, keyCounters] : countersResult.Counters) {
                megaAggregator.Accumulate(key, keyCounters);
            }

            return {.MainKey = mainKey, .FullState = megaAggregator.Output(), .Errors = megaAggregator.GetAndDeleteLogs()};
        }
    }  // namespace NImpl

    void TCollectCountersReducer::Do(TReader *reader, TWriter *writer) {
        NImpl::TCountersResult counters = NImpl::ComputeCounters(Config, begin(*reader), end(*reader));

        if (counters.Errors) {
            Cerr << "puid " << counters.Puid << " counters errors:" << counters.Errors << Endl;
        }

        if (counters.Counters.empty()) {
            return;
        }

        const NImpl::TAggregationResult aggregated = NImpl::AggregateCounters(Config, counters);

        if (aggregated.Errors) {
            Cerr << "puid " << counters.Puid << " aggregation errors:" << aggregated.Errors << Endl;
        }

        TString stateId = aggregated.MainKey.SerializeAsString();
        if (stateId.empty()) {
            Cerr << "puid " << counters.Puid << " error with id serialization" << Endl;
            return;
        }

        TFullState outputRow;
        outputRow.SetPuid(std::move(counters.Puid));
        outputRow.SetStateId(std::move(stateId));
        outputRow.SetState(aggregated.FullState.SerializeAsString());
        outputRow.SetStateDebug(aggregated.FullState.DebugString());

        writer->AddRow(outputRow);
    }

    void TCountersCompactifier::Do(TReader *reader, TWriter *writer) {
        for (const auto& cursor : *reader) {
            const TFullState& fullState = cursor.GetRow();

            NRapidClicks::TAggregatorState fullAggregatorState;
            Y_ENSURE(fullAggregatorState.ParseFromString(fullState.GetState()));

            NRapidClicks::TCompactAggregatorState compactAggregatorState;
            NRapidClicks::CompactifyState(fullAggregatorState, compactAggregatorState, &DomAttrCanonizer);

            TCompactState compactState;

            compactState.SetId(fullState.GetStateId());
            compactState.SetCodec(TString(CODEC));

            TIntrusivePtr<NCodecs::ICodec> codec = NCodecs::ICodec::GetInstance(CODEC);
            TBuffer codecBuffer;
            codec->Encode(compactAggregatorState.SerializeAsString(), codecBuffer);
            codecBuffer.AsString(*compactState.MutableState());

            compactState.SetStateDiff("");
            compactState.SetStatePatch("");

            writer->AddRow(compactState);
        }
    }

    void TCountersCompactifier::PrepareOperation(
        const NYT::IOperationPreparationContext& /*context*/, NYT::TJobOperationPreparer& preparer) const
    {
        preparer
            .OutputDescription<TCompactState>(0, /* inferSchema */ false)
            .OutputSchema(0, CreateCompactTableSchema());
    }

    NYT::TTableSchema CreateCompactTableSchema() {
        NYT::TTableSchema schema = NYT::CreateTableSchema<TCompactState>();

        auto* hashColumnPtr = FindIf(schema.MutableColumns(), [](const auto& columnSchema){ return columnSchema.Name() == "Hash"; });
        Y_ENSURE(hashColumnPtr != schema.MutableColumns().end());

        hashColumnPtr->Expression("bigb_hash(Id) % 256"); // can't set expression in protobuf flags

        return schema;
    }
}  // namespace NAlice::NMusicWebSearch::NListeningCounters

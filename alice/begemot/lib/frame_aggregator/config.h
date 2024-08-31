#pragma once

#include <alice/begemot/lib/frame_aggregator/proto/config.pb.h>
#include <alice/begemot/lib/fresh_options/proto/fresh_options.pb.h>
#include <alice/begemot/lib/utils/config.h>
#include <alice/library/proto/proto.h>

#include <search/begemot/core/filesystem.h>

#include <library/cpp/langs/langs.h>

#include <util/folder/path.h>
#include <util/generic/yexception.h>
#include <util/generic/hash.h>
#include <util/string/cast.h>

namespace NAlice {

enum class EFrameAggregatorSourceName {
    UNKNOWN                             /* "Unknown" */,
    ALWAYS                              /* "Always" */,
    ALICE_ACTION_RECOGNIZER             /* "AliceActionRecognizer" */,
    ALICE_BINARY_INTENT_CLASSIFIER      /* "AliceBinaryIntentClassifier" */,
    ALICE_FIXLIST                       /* "AliceFixlist" */,
    ALICE_FRAME_FILLER                  /* "AliceFrameFiller" */,
    ALICE_MULTI_INTENT_CLASSIFIER       /* "AliceMultiIntentClassifier" */,
    ALICE_SCENARIOS_WORD_LSTM           /* "AliceScenariosWordLstm" */,
    ALICE_TOLOKA_WORD_LSTM              /* "AliceTolokaWordLstm" */,
    ALICE_TAGGER                        /* "AliceTagger" */,
    ALICE_TRIVIAL_TAGGER                /* "AliceTrivialTagger" */,
    ALICE_WIZ_DETECTION                 /* "AliceWizDetection" */,
    GRANET                              /* "Granet" */,
};

inline EFrameAggregatorSourceName ReadFrameAggregatorSourceName(TStringBuf str) {
    return FromStringWithDefault<EFrameAggregatorSourceName>(str, EFrameAggregatorSourceName::UNKNOWN);
}

TFrameAggregatorConfig ReadFrameAggregatorConfigFromProtoTxtString(TStringBuf str);

THashMap<ELanguage, TFrameAggregatorConfig> LoadFrameAggregatorConfigs(const NBg::TFileSystem& fs, const TFsPath& dir);

class TFrameConfigValidationException : public yexception {
};

void Validate(const TFrameAggregatorConfig& config);

TFrameAggregatorConfig MergeFrameAggregatorConfigs(
    const TFrameAggregatorConfig* staticConfig,
    const TFrameAggregatorConfig* freshConfig,
    const TFrameAggregatorConfig* devConfig,
    const TVector<TFrameAggregatorConfig>& configPatches,
    const TFreshOptions& freshOptions);

} // namespace NAlice

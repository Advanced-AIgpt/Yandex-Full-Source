#pragma once

#include <alice/nlu/libs/anaphora_resolver/common/mention.h>
#include <alice/nlu/libs/sample_features/sample_features.h>
#include <util/generic/maybe.h>

namespace NAlice {
    struct TAnaphoraMatcherTestSample {
        TVector<NVins::TSample> DialogHistory;
        TVector<TMentionInDialogue> EntityPositions;
        TMentionInDialogue PronounPosition;
        TVector<bool> ValidEntitiesMarkup;
        TMaybe<TMentionInDialogue> AnswerEntity;
    };
};

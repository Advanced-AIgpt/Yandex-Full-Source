#pragma once
#include "test_sample.h"
#include <alice/nlu/libs/anaphora_resolver/common/mention.h>
#include <alice/nlu/libs/sample_features/sample_features.h>
#include <util/folder/path.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/file.h>

namespace NAlice {
    class TDsvAnaphoraTestReader {
    public:
        TDsvAnaphoraTestReader() = delete;
        explicit TDsvAnaphoraTestReader(THolder<IInputStream>&& input);

        bool HasLine() const;

        bool ParseLine(TAnaphoraMatcherTestSample* testSample, IOutputStream* errorLog);

    private:
        void TryPullNextLine();

    private:
        THolder<IInputStream> Input;
        bool CurrentLineExists = true;
        TString CurrentLine;
    };
} // namespace NAlice

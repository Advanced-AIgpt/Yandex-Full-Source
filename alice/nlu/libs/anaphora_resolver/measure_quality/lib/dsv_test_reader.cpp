#include "dsv_test_reader.h"
#include <util/string/split.h>

namespace NAlice {
    namespace {
        constexpr TStringBuf DIALOG_PHRASES_SEPARATOR = " [SEP] ";

        bool ParseDialogHistory(const TString& line,
                                TVector<NVins::TSample>* dialogHistory,
                                TVector<size_t>* tokenPosInPhrase,
                                TVector<size_t>* phraseIdxForToken,
                                IOutputStream* errorLog) {
            Y_ASSERT(dialogHistory);
            Y_ASSERT(tokenPosInPhrase);
            Y_ASSERT(phraseIdxForToken);
            Y_ASSERT(errorLog);

            const TVector<TString> dialogPhrases = StringSplitter(line).SplitByString(DIALOG_PHRASES_SEPARATOR);
            if (dialogPhrases.size() == 0) {
                *errorLog << "Empty dialog.";
                return false;
            }

            dialogHistory->reserve(dialogPhrases.size());
            for (size_t phraseIdx = 0; phraseIdx < dialogPhrases.size(); ++phraseIdx) {
                const auto& phrase = dialogPhrases[phraseIdx];

                NVins::TSample sample;
                sample.Text = phrase;
                sample.Tokens = StringSplitter(phrase).Split(' ');
                dialogHistory->push_back(sample);

                phraseIdxForToken->insert(phraseIdxForToken->end(), sample.Tokens.size() + /*[SEP]*/1, phraseIdx);
                const size_t totalTokensCount = tokenPosInPhrase->size();
                tokenPosInPhrase->insert(tokenPosInPhrase->end(), sample.Tokens.size() + /*[SEP]*/1, 0);
                Iota(tokenPosInPhrase->begin() + totalTokensCount, tokenPosInPhrase->end(), 0);
            }

            return true;
        }

        bool ParseEntitySegments(const TString& line,
                                 const TVector<size_t>& tokenPosInPhrase,
                                 const TVector<size_t>& phraseIdxForToken,
                                 TVector<TMentionInDialogue>* entities,
                                 TMentionInDialogue* pronoun,
                                 IOutputStream* errorLog) {
            Y_ASSERT(entities);
            Y_ASSERT(pronoun);
            Y_ASSERT(errorLog);

            const TVector<TString> entitySegmentStrings = StringSplitter(line).Split(' ');
            if (entitySegmentStrings.empty()) {
                *errorLog << "No pronoun.";
                return false;
            }

            for (size_t segmentIdx = 0; segmentIdx < entitySegmentStrings.size(); ++segmentIdx) {
                const auto& segmentString = entitySegmentStrings[segmentIdx];

                size_t start;
                size_t end;
                bool correctSegment = StringSplitter(segmentString).Split(',').TryCollectInto(&start, &end);
                if (correctSegment) {
                    correctSegment = start < end && end <= phraseIdxForToken.size() &&
                                     phraseIdxForToken[start] == phraseIdxForToken[end];
                }
                if (!correctSegment) {
                    *errorLog << "Unable to extract entity segments.";
                    return false;
                }

                TMentionInDialogue mention(phraseIdxForToken[start], tokenPosInPhrase[start], tokenPosInPhrase[end]);

                if (segmentIdx == entitySegmentStrings.size() - 1) {
                    *pronoun = mention;
                } else {
                    entities->push_back(mention);
                }
            }

            return true;
        }

        bool ParseValidEntitiesMarkup(const TString& line, TVector<bool>* validEntitiesMarkup, IOutputStream* errorLog) {
            Y_ASSERT(validEntitiesMarkup);
            Y_ASSERT(errorLog);

            if (line.empty()) {
                return true;
            }
            const TVector<TString> validEntitiesMarkupStrings = StringSplitter(line).Split(' ');

            validEntitiesMarkup->reserve(validEntitiesMarkupStrings.size());
            for (const auto& markString : validEntitiesMarkupStrings) {
                bool mark;
                if (!TryFromString<bool>(markString, mark)) {
                    *errorLog << "Unable to parse validity mark '" << markString << "'.";
                    return false;
                }
                validEntitiesMarkup->push_back(mark);
            }

            return true;
        }

        bool ParseTestLine(const TString& testLine, TAnaphoraMatcherTestSample* testSample, IOutputStream* errorLog) {
            Y_ASSERT(testSample);
            Y_ASSERT(errorLog);

            TVector<TString> parts = StringSplitter(testLine).Split('\t');
            if (parts.size() < 3) { // dialog, entities and valid entities markup
                *errorLog << "Not enough columns.";
                return false;
            }

            TVector<size_t> tokenPosInPhrase;
            TVector<size_t> phraseIdxForToken;
            if (!ParseDialogHistory(parts[0], &testSample->DialogHistory, &tokenPosInPhrase, &phraseIdxForToken, errorLog)) {
                return false;
            }

            if (!ParseEntitySegments(parts[1], tokenPosInPhrase, phraseIdxForToken,
                                     &testSample->EntityPositions, &testSample->PronounPosition, errorLog)) {
                return false;
            }

            if (!ParseValidEntitiesMarkup(parts[2], &testSample->ValidEntitiesMarkup, errorLog)) {
                return false;
            }

            if (testSample->ValidEntitiesMarkup.size() != testSample->EntityPositions.size()) {
                *errorLog << "Valid entities markup does not match entities count.";
                return false;
            }

            return true;
        }
    } // namespace anonynous

    TDsvAnaphoraTestReader::TDsvAnaphoraTestReader(THolder<IInputStream>&& input)
        : Input(input.Release()) {
        TryPullNextLine();
    }

    bool TDsvAnaphoraTestReader::HasLine() const {
        return CurrentLineExists;
    }

    bool TDsvAnaphoraTestReader::ParseLine(TAnaphoraMatcherTestSample* testSample, IOutputStream* errorLog) {
        Y_ENSURE(HasLine());
        const bool parsed = ParseTestLine(CurrentLine, testSample, errorLog);
        TryPullNextLine();
        return parsed;
    }

    void TDsvAnaphoraTestReader::TryPullNextLine() {
        if (!CurrentLineExists) {
            return;
        }
        CurrentLineExists = Input->ReadLine(CurrentLine);
    }
} // namespace NAlice

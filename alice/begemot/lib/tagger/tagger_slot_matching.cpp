#include "tagger_slot_matching.h"

#include <search/begemot/rules/alice/tagger/proto/alice_tagger.pb.h>

#include <alice/begemot/lib/utils/form_to_frame.h>

#include <alice/library/frame/description.h>
#include <alice/library/json/json.h>
#include <alice/library/request/token_range.h>

#include <util/generic/algorithm.h>
#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/string/join.h>
#include <util/string/split.h>
#include <util/system/yassert.h>

#include <tuple>

namespace NAlice {
    namespace {

        const THashSet<TStringBuf> EXCEPTIONALLY_PROCESSED_INTENTS{
            NVideoCommon::SEARCH_VIDEO,
            NVideoCommon::SEARCH_VIDEO_TEXT,
            NVideoCommon::QUASAR_SELECT_VIDEO_FROM_GALLERY_BY_TEXT,
            NVideoCommon::QUASAR_SELECT_VIDEO_FROM_GALLERY_BY_TEXT_TEXT,
        };

        TString GetSourceText(const TVector<TString>& tokens, int Begin, int End) {
            TString result;
            Y_ASSERT(End <= tokens.ysize());
            End = Min(End, tokens.ysize());
            for (int i = Begin; i < End; ++i) {
                result.append(tokens[i]);
                if (i < End - 1) {
                    result.append(" ");
                }
            }
            return result;
        }

        bool IsCorrectType(const TString& target, const TString& type, const TStringBuf prefix) {
            return (
                target.StartsWith(prefix) &&
                target.EndsWith(type) &&
                target.length() == type.length() + prefix.length()
            ) || target == type;
        }

        TSlot BoostSlotValue(const TSlot& oldSlot, const TString& newValue, const TString& sourceText) {
            const TString& type = oldSlot.Type;
            if (const auto* valuesList = SLOT_VALUES_PRIORITY_LIST.FindPtr(type)) {
                for (const auto& value : *valuesList) {
                    if (value == oldSlot.Value.AsString()) {
                        return oldSlot;
                    }
                    if (value == newValue) {
                        return TSlot(oldSlot.Name, oldSlot.Type, newValue, sourceText);
                    }
                }
            }
            return oldSlot;
        }

        TMaybe<TSlot> TryGetTypedSlot(
            const NBg::NProto::TGranetTag& tag,
            const TString& type,
            const TStringBuf name,
            const TVector<TString>& tokens
        ) {
            TMaybe<TSlot> result;
            for (const auto& variant : tag.GetData()) {
                const auto& variantType = variant.GetType();
                if (!IsCorrectType(variantType, type, FST_TYPE_PREFIX) &&
                    !IsCorrectType(variantType, type, CUSTOM_TYPE_PREFIX)) {
                    continue;
                }

                if (!result.Defined()) {
                    result = TSlot(TString{name}, type, variant.GetValue(),
                                   GetSourceText(tokens, variant.GetBegin(), variant.GetEnd()));
                } else {
                    result = BoostSlotValue(*result, variant.GetValue(),
                                            GetSourceText(tokens, variant.GetBegin(), variant.GetEnd()));
                }
            }
            return result;
        }

        bool TagSupportsConcatenation(TStringBuf tag, const TFrameDescription* frameDescription) {
            if (!frameDescription) {
                return false;
            }
            const auto* slot = frameDescription->Slots.FindPtr(tag);
            if (!slot) {
                return false;
            }
            return slot->ConcatenateStrings;
        }

        struct TToken {
            enum class EType { Begin, Inner, Other };

            TToken(const TString& tag, const TString& text, ui32 pos, bool supportsConcatenation = false)
                : Text(text)
                , Range{pos, pos + 1}
                , SupportsConcatenation{supportsConcatenation} {
                if (tag.size() < 2) {
                    return;
                }
                if (tag[0] == 'B') {
                    Type = EType::Begin;
                } else if (tag[0] == 'I') {
                    Type = EType::Inner;
                } else {
                    Type = EType::Other;
                }
                Tag = tag.substr(2);
            }

            bool Append(const TToken& other) {
                Y_ASSERT(Tag == other.Tag);

                if (Range.End != other.Range.Start && !SupportsConcatenation)
                    return false;

                Text.append(" ").append(other.Text);
                Range.End = other.Range.End;

                return true;
            }

            EType Type;
            TString Tag;
            TString Text;
            TTokenRange Range;
            bool SupportsConcatenation = false;
        };

        using TTokens = THashMap<TString, TVector<TToken>>;

        TTokens ExtractAliceTaggerTokens(const TFrameDescription* frameDescription,
                                         const google::protobuf::RepeatedPtrField<NBg::NProto::TAliceTaggerToken>& tokens) {
            TTokens parsedTokens;
            for (int tokenPos = 0; tokenPos < tokens.size(); ++tokenPos) {
                const auto& token = tokens[tokenPos];
                const auto& tag = token.GetTag();
                const auto& text = token.GetText();

                if (tag == "O") // The token doesn't correspond to any feature.
                    continue;

                TStringBuf slotName = tag;
                slotName.Skip(2); // skip prefix "B-" and "I-"
                TToken parsedToken{tag, text, static_cast<ui32>(tokenPos), TagSupportsConcatenation(slotName, frameDescription)};

                if (auto* ptr = parsedTokens.FindPtr(parsedToken.Tag)) {
                    Y_ASSERT(!ptr->empty());
                    if (!ptr->back().Append(parsedToken)) {
                        ptr->push_back(parsedToken);
                    }
                } else {
                    parsedTokens[parsedToken.Tag].emplace_back(std::move(parsedToken));
                }
            }
            return parsedTokens;
        }

        bool TokenMatchRecognition(const TToken& token, const TRecognition& recognition) {
            return recognition.Range.Start >= token.Range.Start && recognition.Range.End <= token.Range.End;
        }

        struct TCropResult {
            TString Text;
            TTokenRange Range;
        };

        TCropResult CropText(const TToken& token, const TTokenRange& range) {
            if (token.Range == range) {
                return {token.Text, token.Range};
            }

            const TVector<TStringBuf> splittedTokens = StringSplitter(token.Text).Split(' ');

            const ui32 start = token.Range.Start >= range.Start ? token.Range.Start - range.Start : 0;
            const ui32 end = std::min(token.Range.End, range.End) - std::max(token.Range.Start, range.Start);
            Y_ASSERT(start < splittedTokens.size());
            Y_ASSERT(end <= splittedTokens.size());

            return {JoinRange(" ", splittedTokens.begin() + start, splittedTokens.begin() + end), range};
        }

        TMaybe<TSlot> FeatureToSlot(const TFrameDescription* frameDescription, const TToken& token, const TCollectedEntities& entities) {
            if (!frameDescription) {
                return {};
            }

            const auto* slot = frameDescription->Slots.FindPtr(token.Tag);
            if (!slot) {
                return {};
            }

            for (const auto& type : slot->Types) {
                if (const auto* recognitions = entities.FindPtr(type)) {
                    for (const auto& recognition : *recognitions) {
                        if (TokenMatchRecognition(token, recognition)) {
                            auto cropResut = CropText(token, recognition.Range);
                            return TSlot{token.Tag, TString{type}, recognition.Value, cropResut.Text,
                                        cropResut.Range};
                        }
                    }
                }

                if (type == SLOT_STRING_TYPE) {
                    return TSlot{token.Tag, TString{type}, token.Text, token.Text, token.Range};
                }

                if (type == SLOT_NUM_TYPE) {
                    ui32 value = 0;
                    if (TryFromString(token.Text, value)) {
                        return TSlot{token.Tag, TString{type}, token.Text, token.Text, token.Range};
                    }
                }
            }

            return {};
        }


        // ------------------------------ ALTERNATIVE MATCHING LOGIC ------------------------------

        class TTaggerSlot {
        public:
            enum class EType { Begin, Inner, Other };

            TTaggerSlot(const TString& tag, TString text, ui32 pos);
            void Extend(const TTaggerSlot& other);
            EType GetType() const;
            TString GetTag() const;
            TString GetText() const;
            const NAlice::TTokenRange& GetRange() const;

        private:
            EType Type;
            TString Tag;
            TString Text;
            NAlice::TTokenRange Range;
        };

        TTaggerSlot::TTaggerSlot(const TString& tag, TString text, ui32 pos): Text(std::move(text)), Range{pos, pos + 1} {
            if (tag.size() < 2) {
                Type = EType::Other;
                return;
            }
            if (tag[0] == 'B') {
                Type = EType::Begin;
            } else if (tag[0] == 'I') {
                Type = EType::Inner;
            } else {
                Type = EType::Other;
            }
            Tag = tag.substr(2);
        }

        TTaggerSlot::EType TTaggerSlot::GetType() const {
            return Type;
        }

        TString TTaggerSlot::GetTag() const {
            return Tag;
        }

        TString TTaggerSlot::GetText() const {
            return Text;
        }
        const NAlice::TTokenRange& TTaggerSlot::GetRange() const {
            return Range;
        }

        void TTaggerSlot::Extend(const TTaggerSlot& other) {
            Y_ASSERT(Tag == other.Tag);
            Text.append(" ").append(other.Text);
            // TODO(movb): For some tag types (search_text) tags can be located in different part of utterance,
            // but for others it would be nice to check that current End equals Begin of next token
            Range.End = other.Range.End;
        }

        using TProtoTokens = google::protobuf::RepeatedPtrField<NBg::NProto::TAliceTaggerToken>;

        THashMap<TString, TTaggerSlot> ExtractAliceTaggerTokens(const TProtoTokens& tokens) {
            THashMap<TString, TTaggerSlot> parsedTokens;
            for (int tokenPos = 0; tokenPos < tokens.size(); ++tokenPos) {
                const auto& token = tokens[tokenPos];
                const auto& tag = token.GetTag();
                const auto& text = token.GetText();

                if (tag == "O") {// The token doesn't correspond to any feature.
                    continue;
                }
                TTaggerSlot parsedToken(tag, text, static_cast<ui32>(tokenPos));

                if (auto* ptr = parsedTokens.FindPtr(parsedToken.GetTag())) {
                    ptr->Extend(parsedToken);
                } else {
                    parsedTokens.emplace(parsedToken.GetTag(), parsedToken);
                }
            }
            return parsedTokens;
        }

        void AddSlotValue(
            const TString& type,
            const TString& value,
            size_t begin,
            size_t end,
            NBg::NProto::TGranetTag* tag
        ) {
            NBg::NProto::TGranetSlotValue* slotValue = tag->AddData();
            slotValue->SetBegin(begin);
            slotValue->SetEnd(end);
            slotValue->SetType(type);
            slotValue->SetValue(value);
        }

        void MakeSlotsFromPrediction(
            const NBg::NProto::TAliceTaggerPrediction& prediction,
            const TVector<NGranet::TEntity>& entities,
            NBg::NProto::TGranetForm* form
        ) {
            const auto& slots = ExtractAliceTaggerTokens(prediction.GetToken());
            for (const auto& [tagType, tag] : slots) {
                NBg::NProto::TGranetTag* currentTag = form->AddTags();
                currentTag->SetBegin(tag.GetRange().Start);
                currentTag->SetEnd(tag.GetRange().End);
                currentTag->SetName(tagType);
                AddSlotValue("string", tag.GetText(), tag.GetRange().Start, tag.GetRange().End, currentTag);
                for (const auto& entity : entities) {
                    if (entity.Interval.Begin < tag.GetRange().End && entity.Interval.End > tag.GetRange().Start) {
                        AddSlotValue(entity.Type, entity.Value, entity.Interval.Begin, entity.Interval.End, currentTag);
                    }
                }
            }
        }

        void FillForm(
            const TString& formName,
            const TSlotMap& slotMap,
            double probability,
            NBg::NProto::TGranetForm* form
        ) {
            form->SetName(formName);
            form->SetLogProbability(probability);
            for (const auto& [tag, slot] : slotMap) {
                NBg::NProto::TGranetTag* currentTag = form->AddTags();
                currentTag->SetBegin(slot.Range.Start);
                currentTag->SetEnd(slot.Range.End);
                currentTag->SetName(slot.Name);
                AddSlotValue(slot.Type, slot.Value.AsString(), slot.Range.Start, slot.Range.End, currentTag);
            }
        }

        void SortSlots(TSemanticFrame& frame) {
            SortBy(*frame.MutableSlots(), [](const TSemanticFrame::TSlot& slot) {
                return std::tie(slot.GetName(), slot.GetType(), slot.GetValue());
            });
        }
    } // namespace


    TTaggerPrediction::TTaggerPrediction(
        TStringBuf taggerName,
        const ::google::protobuf::RepeatedPtrField<NBg::NProto::TAliceTaggerPrediction>& protoPredictions,
        const TCollectedEntities& entities,
        const TFrameDescription* frameDescription
    )
        : Name(taggerName)
    {
        for (const auto& prediction : protoPredictions) {
            TTokens parsedTokens = ExtractAliceTaggerTokens(frameDescription, prediction.GetToken());

            TSlotMap result;
            result.reserve(parsedTokens.size());

            bool hasMismatchedSlots = false;
            for (const auto& [tag, tokens] : parsedTokens) {
                TMaybe<TSlot> maxLengthSlot;
                for (const auto& token : tokens) {
                    if (auto slot = FeatureToSlot(frameDescription, token, entities)) {
                        if (!maxLengthSlot.Defined() || slot->Range.Size() > maxLengthSlot->Range.Size()) {
                            maxLengthSlot = slot;
                        }
                    }
                }
                if (maxLengthSlot) {
                    result[tag] = *maxLengthSlot;
                } else {
                    hasMismatchedSlots = true;
                }
            }

            if (hasMismatchedSlots) {
                continue;
            }

            Probability = prediction.GetProbability();
            SlotMap = std::move(result);
            break;
        }
    }
    
    const TString& TTaggerPrediction::GetName() const {
        return Name;
    }

    const TSlotMap& TTaggerPrediction::GetSlotMap() const {
        return SlotMap;
    }

    TVector<TString> ExtractSourceTokens(const NBg::NProto::TAliceTaggerPrediction& prediction) {
        TVector<TString> result;
        for (const auto& token : prediction.GetToken()) {
            result.push_back(token.GetText());
        }
        return result;
    }

    TSlotMap FormToSlotMap(const NBg::NProto::TGranetForm& form,
                           const TVector<TString>& tokens,
                           const TFrameDescription* frameDescription) {
        TSlotMap result;
        if (!frameDescription) {
            return result;
        }
        for (const auto& tag : form.GetTags()) {
            const auto& slotName = tag.GetName();
            const auto* formSlot = frameDescription->Slots.FindPtr(slotName);
            if (!formSlot) {
                continue;
            }
            for (const auto& type : formSlot->Types) {
                const auto slot = TryGetTypedSlot(tag, TString{type}, slotName, tokens);
                if (slot.Defined()) {
                    result.emplace(slotName, slot.GetRef());
                    break;
                }
            }
        }
        return result;
    }

    void FillFormAndFrame(
        const NBg::NProto::TAliceTaggerPredictions& predictions,
        const TVector<NGranet::TEntity>& granetEntities,
        const TCollectedEntities& colllectedEntities,
        const TString& taggerName,
        const TFrameDescription* frameDescription,
        const THashSet<TString>& intentsToForceRegularProcessing,
        NBg::NProto::TGranetForm* form,
        TSemanticFrame* frame
    ) {
        Y_ASSERT(form);
        Y_ASSERT(frame);
        
        if (NAlice::EXCEPTIONALLY_PROCESSED_INTENTS.contains(taggerName) && !intentsToForceRegularProcessing.contains(taggerName)) {
            const NBg::NProto::TAliceTaggerPrediction& prediction = predictions.GetPrediction(0);
            form->SetName(taggerName);
            form->SetLogProbability(prediction.GetProbability());
            MakeSlotsFromPrediction(prediction, granetEntities, form);
            const TVector<TString> tokens = NAlice::ExtractSourceTokens(prediction);
            NAlice::TSlotMap slotMap = NAlice::FormToSlotMap(*form, tokens, frameDescription);
            *form = NBg::NProto::TGranetForm{};
            FillForm(taggerName, slotMap, prediction.GetProbability(), form);
            *frame = ToSemanticFrame(taggerName, slotMap);
        } else {
            const NAlice::TTaggerPrediction taggerPrediction{
                taggerName,
                predictions.GetPrediction(),
                colllectedEntities,
                frameDescription
            };
            FillForm(taggerName, taggerPrediction.GetSlotMap(), taggerPrediction.GetProbability().GetOrElse(0.0), form);
            *frame = ToSemanticFrame(taggerName, taggerPrediction.GetSlotMap());
        }
    }

    void FillFormsAndFrames(
        const ::google::protobuf::Map<TString, NBg::NProto::TAliceTaggerPredictions>& taggerPredictions,
        const TVector<NGranet::TEntity>& granetEntities,
        const NBg::NProto::TCustomEntitiesResult& customEntities,
        const NBg::NProto::TAliceEntitiesCollectorResult& aliceEntitiesCollectorResult,
        const THashMap<TString, TFrameDescription>& frameDescriptionMap,
        const THashSet<TString>& intentsToForceRegularProcessing,
        NBg::NProto::TAliceTaggerResult* result
    ) {
        TVector<TString> taggerNames(Reserve(taggerPredictions.size()));
        for (const auto& [taggerName, _] : taggerPredictions) {
            taggerNames.push_back(taggerName);
        }
        Sort(taggerNames);

        for (const auto& taggerName : taggerNames) {
            const auto& predictions = taggerPredictions.at(taggerName);
            if (predictions.GetPrediction().empty()) {
                continue;
            }

            const TFrameDescription* frameDescription = frameDescriptionMap.FindPtr(taggerName);
            if (!frameDescription) {
                continue;
            }

            NBg::NProto::TGranetForm* form = result->AddForms();
            NAlice::TSemanticFrame* frame = result->AddFrames();
            FillFormAndFrame(
                predictions,
                granetEntities,
                NAlice::CollectEntities(customEntities, aliceEntitiesCollectorResult),
                taggerName,
                frameDescription,
                intentsToForceRegularProcessing,
                form,
                frame
            );
            SortSlots(*frame);
        }
    }
}

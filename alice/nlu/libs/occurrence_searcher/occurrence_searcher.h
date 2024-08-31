#pragma once
#include <library/cpp/containers/comptrie/pattern_searcher.h>
#include <library/cpp/containers/comptrie/protopacker.h>
#include <library/cpp/langs/langs.h>
#include <library/cpp/packers/packers.h>
#include <google/protobuf/message.h>
#include <util/generic/flags.h>
#include <util/generic/hash.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/memory/blob.h>
#include <util/string/join.h>
#include <util/system/types.h>
#include <util/ysaveload.h>
#include <type_traits>

namespace NAlice {
namespace NNlu {
    template<typename TValue>
    void Update(TValue* value, const TValue& newValue) {
        Y_ASSERT(value);
        (*value) = newValue;
    }


    template<typename TValue>
    struct TOccurrence {
        TValue Value = TValue();
        ui64 Begin = 0;
        ui64 End = 0;
    };


    template<typename TValue>
    struct TItem {
        size_t Length = 0;
        TValue Value = TValue();
    };


    template <typename TValue>
    class TItemPacker {
        using TLengthPacker = NPackers::TPacker<size_t>;
        using TValuePacker = std::conditional_t<
            std::is_base_of_v<google::protobuf::Message, TValue>,
            TProtoPacker<TValue>,
            NPackers::TPacker<TValue>
        >;
    public:
        inline void UnpackLeaf(const char* buffer, TItem<TValue>& item) const {
            TLengthPacker().UnpackLeaf(buffer, item.Length);
            buffer += TLengthPacker().SkipLeaf(buffer);
            TValuePacker().UnpackLeaf(buffer, item.Value);
        }

        inline void PackLeaf(char* buffer, const TItem<TValue>& item, size_t) const {
            size_t lengthSize = TLengthPacker().MeasureLeaf(item.Length);
            TLengthPacker().PackLeaf(buffer, item.Length, lengthSize);
            size_t valueSize = TValuePacker().MeasureLeaf(item.Value);
            TValuePacker().PackLeaf(buffer + lengthSize, item.Value, valueSize);
        }

        inline size_t MeasureLeaf(const TItem<TValue>& item) const {
            return TLengthPacker().MeasureLeaf(item.Length) +
                   TValuePacker().MeasureLeaf(item.Value);
        }

        inline size_t SkipLeaf(const char* buffer) const {
            size_t lengthSize = TLengthPacker().SkipLeaf(buffer);
            size_t valueSize = TValuePacker().SkipLeaf(buffer + lengthSize);
            return lengthSize + valueSize;
        }
    };


    template<typename TValue>
    class TOccurrenceSearcher {
    public:
        explicit TOccurrenceSearcher(const TBlob& searcherData)
            : AutomatonData(searcherData)
            , Automaton(AutomatonData)
        { }

        TVector<TOccurrence<TValue>> Search(const TString& string) const {
            const auto& searchResult = Automaton.SearchMatches(" " + string + " ");
            TVector<TOccurrence<TValue>> occurences(Reserve(searchResult.size()));
            for (const auto& item : searchResult) {
                TOccurrence<TValue> occurrence;
                occurrence.End = item.End - 1;
                occurrence.Begin = occurrence.End - item.Data.Length;
                occurrence.Value = item.Data.Value;
                occurences.push_back(occurrence);
            }
            return occurences;
        }

    private:
        TBlob AutomatonData;
        TCompactPatternSearcher<char, TItem<TValue>, TItemPacker<TValue>> Automaton;
    };


    template<typename TValue>
    class TTokenizedOccurrenceSearcher {
    public:
        explicit TTokenizedOccurrenceSearcher(const TBlob& searcherData)
            : OccurrenceSearcher(searcherData)
        { }

        TVector<TOccurrence<TValue>> Search(const TVector<TString>& tokens) const {
            TVector<TOccurrence<TValue>> result;
            THashMap<size_t, size_t> beginByteToTokenIndex;
            THashMap<size_t, size_t> endByteToTokenIndex;
            size_t beginBytePos = 0;
            for (size_t tokenIndex = 0; tokenIndex < static_cast<size_t>(tokens.size()); ++tokenIndex) {
                const TString& token = tokens[tokenIndex];

                beginByteToTokenIndex[beginBytePos] = tokenIndex;
                endByteToTokenIndex[beginBytePos + token.length()] = tokenIndex + 1;

                beginBytePos += 1 + token.length();
            }

            const TString text = JoinSeq(" ", tokens);

            for (const TOccurrence<TValue>& occurrence : OccurrenceSearcher.Search(text)) {
                result.push_back(TOccurrence<TValue>{
                    .Value = occurrence.Value,
                    .Begin = beginByteToTokenIndex.at(occurrence.Begin),
                    .End = endByteToTokenIndex.at(occurrence.End)
                });
            }

            return result;
        }

    private:
        TOccurrenceSearcher<TValue> OccurrenceSearcher;
    };


    template<typename TValue>
    class TOccurrenceSearcherDataBuilder {
    public:
        void Add(const TString& key, const TValue& value) {
            Update(&KeyValueMap[key], value);
        }

        TBlob Build() {
            TCompactPatternSearcherBuilder<char, TItem<TValue>, TItemPacker<TValue>> builder;
            for (const auto& pair : KeyValueMap) {
                const auto& key = pair.first;
                const auto& value = pair.second;
                builder.Add(" " + key + " ", TItem<TValue>{key.length(), value});
            }
            return builder.Save();
        }

    private:
        THashMap<TString, TValue> KeyValueMap;
    };

    TString NormalizeString(ELanguage language, TStringBuf string);
} // NNlu
} // NAlice

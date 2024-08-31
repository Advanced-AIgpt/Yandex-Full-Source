#pragma once

#include <alice/nlu/libs/interval/interval.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NNlu {

// ~~~~ TAlignmentData ~~~~

struct TAlignmentData {
    // Segmentations of aligned texts.
    // SegmentsN[i] - number of tokens in i-th segment of text N.
    // Arrays are parallel: segment Segments1[i] correspond to Segments2[i].
    // Example (from unit tests):
    //   Text 1 segmentation:   "Сколько|будет|минус|сто двадцать три|плюс|пятьдесят семь"
    //   Text 2 segmentation:   "сколько|будет|-    |123             |+   |57"
    //   Segments1:             {1,      1,    1,    3,               1,   2}
    //   Segments2:             {1,      1,    1,    1,               1,   1}
    TVector<size_t> Segments1;
    TVector<size_t> Segments2;
    TVector<bool> Equality;

    // Create trivial alignment for equal texts.
    // count - token count in text.
    static TAlignmentData CreateTrivial(size_t count);

    bool IsValid() const;
    bool IsEmpty() const;

    void AddSegment(size_t segment1, size_t segment2, bool areEqual);
    void AddOneToOneSegments(size_t count, bool areEqual);

    size_t CountTokens1() const;
    size_t CountTokens2() const;

    // groups - lengths of merged token groups.
    void MergeTokens1(const TVector<size_t>& groups);
    void MergeTokens2(const TVector<size_t>& groups);

    void MergeSegmentWithNext(size_t segmentIndex);

    TString WriteToString() const;
    static TAlignmentData ReadFromString(TStringBuf str);
};

inline bool operator==(const TAlignmentData& data1, const TAlignmentData& data2) {
    return data1.Segments1 == data2.Segments1
        && data1.Segments2 == data2.Segments2
        && data1.Equality == data2.Equality;
}

inline IOutputStream& operator<<(IOutputStream& out, const TAlignmentData& data) {
    out << data.WriteToString();
    return out;
}

// ~~~~ TAlignmentMap ~~~~

class TAlignmentMap {
public:
    TAlignmentMap() = default;
    TAlignmentMap(const TVector<size_t>& srcSegments, const TVector<size_t>& destSegments,
        const TVector<bool>& equality);

    // Be careful: method can return empty interval.
    NNlu::TInterval ConvertInterval(const NNlu::TInterval& interval) const;

    bool GetEqualTokenCount(const NNlu::TInterval& interval) const;
    bool HasSureMatch(const NNlu::TInterval& interval) const;
    bool HasStrictMatch(const NNlu::TInterval& interval) const;

private:
    NNlu::TInterval MakeSrcIntervalSafe(const NNlu::TInterval& rawInterval) const;

private:
    TVector<NNlu::TInterval> Map;
    TVector<i32> EqualityIntegral;
};

// ~~~~ TAlignment ~~~~

// Alignment of two texts.
// Result of TTokenAligner and TTokenCachedAligner.
class TAlignment {
public:
    TAlignment() = default;
    explicit TAlignment(TAlignmentData&& data);

    // Create trivial alignment for equal texts.
    // count - token count in text.
    static TAlignment CreateTrivial(size_t count);

    const TAlignmentData& GetData() const {
        return Data;
    }
    bool IsEmpty() const {
        return Data.IsEmpty();
    }
    const TVector<size_t>& GetSegments1() const {
        return Data.Segments1;
    }
    const TVector<size_t>& GetSegments2() const {
        return Data.Segments2;
    }

    // Coordinate converters:
    //   interval in 1st array of tokens  ->  interval in 2nd array of tokens
    const TAlignmentMap& GetMap1To2() const {
        return Map1To2;
    }
    //   interval in 2nd array of tokens  ->  interval in 1st array of tokens
    const TAlignmentMap& GetMap2To1() const {
        return Map2To1;
    }

    TString WriteToString() const;
    static TAlignment ReadFromString(TStringBuf str);

private:
    TAlignmentData Data;
    TAlignmentMap Map1To2;
    TAlignmentMap Map2To1;
};

} // namespace NNlu

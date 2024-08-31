#include "alignment.h"
#include <library/cpp/iterator/enumerate.h>
#include <util/generic/reserve.h>
#include <util/string/cast.h>
#include <util/string/split.h>

namespace NNlu {

// ~~~~ TAlignmentData ~~~~

// static
TAlignmentData TAlignmentData::CreateTrivial(size_t count) {
    return {
        .Segments1 = TVector<size_t>(count, 1),
        .Segments2 = TVector<size_t>(count, 1),
        .Equality = TVector<bool>(count, true)
    };
}

bool TAlignmentData::IsValid() const {
    const size_t count = Segments1.size();
    return Segments2.size() == count && Equality.size() == count;
}

bool TAlignmentData::IsEmpty() const {
    return Segments1.empty() && Segments2.empty() && Equality.empty();
}

void TAlignmentData::AddSegment(size_t segment1, size_t segment2, bool areEqual) {
    Segments1.push_back(segment1);
    Segments2.push_back(segment2);
    Equality.push_back(areEqual);
}

void TAlignmentData::AddOneToOneSegments(size_t count, bool areEqual) {
    Segments1.insert(Segments1.end(), count, 1);
    Segments2.insert(Segments2.end(), count, 1);
    Equality.insert(Equality.end(), count, areEqual);
}

size_t TAlignmentData::CountTokens1() const {
    return Accumulate(Segments1, 0u);
}

size_t TAlignmentData::CountTokens2() const {
    return Accumulate(Segments2, 0u);
}

void TAlignmentData::MergeTokens1(const TVector<size_t>& groups) {
    Y_ENSURE(IsValid());
    Y_ENSURE(CountTokens1() == Accumulate(groups, 0u));

    // Optimization for trivial case
    if (AllOf(groups, [](const size_t group) { return group == 1u; })) {
        return;
    }

    size_t segmentCount = Segments1.size();
    size_t segmentIndex = 0;
    size_t segmentStart = 0;

    for (const auto [groupStart, groupLength] : Enumerate(groups)) {
        const size_t groupEnd = groupStart + groupLength;

        while (segmentIndex < segmentCount) {
            size_t& segmentLength = Segments1[segmentIndex];
            const size_t segmentEnd = segmentStart + segmentLength;

            // Skip segments before group
            if (groupStart >= segmentEnd) {
                segmentStart += segmentLength;
                segmentIndex++;
                continue;
            }
            // If group is inside segment, "replace" groupLength tokens in segment by 1 token.
            if (groupEnd <= segmentEnd) {
                segmentLength = segmentLength + 1 - groupLength;
                break;
            }
            // If group begins in one segment and ends in another segment, merge segments.
            Y_ENSURE(segmentIndex + 1 < segmentCount);
            MergeSegmentWithNext(segmentIndex);
            segmentCount--;
        }
    }
    Y_ENSURE(Segments1.size() == segmentCount);
    Y_ENSURE(Segments2.size() == segmentCount);
    Y_ENSURE(Equality.size() == segmentCount);
    Y_ENSURE(CountTokens1() == groups.size());
}

void TAlignmentData::MergeTokens2(const TVector<size_t>& groups) {
    std::swap(Segments1, Segments2);
    MergeTokens1(groups);
    std::swap(Segments2, Segments1);
}

void TAlignmentData::MergeSegmentWithNext(size_t index) {
    Y_ENSURE(IsValid());

    const size_t next = index + 1;
    Y_ENSURE(next < Segments1.size());

    Segments1[index] += Segments1[next];
    Segments2[index] += Segments2[next];
    Equality[index] = Equality[index] && Equality[next];

    Segments1.erase(Segments1.begin() + next);
    Segments2.erase(Segments2.begin() + next);
    Equality.erase(Equality.begin() + next);
}

TString TAlignmentData::WriteToString() const {
    Y_ENSURE(IsValid());
    TString result;
    for (size_t i = 0; i < Segments1.size(); ++i) {
        const size_t from = Segments1[i];
        const size_t to = Segments2[i];
        if (!result.empty()) {
            result += '/';
        }
        const char sign = Equality[i] ? '=' : '~';
        if (from == 1 && to == 1) {
            result += sign;
        } else {
            result += ToString(from);
            result += sign;
            result += ToString(to);
        }
    }
    return result;
}

// static
TAlignmentData TAlignmentData::ReadFromString(TStringBuf str) {
    TAlignmentData self;
    TStringBuf token;
    while (str.NextTok('/', token)) {
        if (token.size() == 1) {
            self.Segments1.push_back(1);
            self.Segments2.push_back(1);
            self.Equality.push_back(token[0] == '=');
            continue;
        }
        TStringBuf left;
        TStringBuf right;
        if (token.TrySplit('=', left, right)) {
            self.Equality.push_back(true);
        } else if (token.TrySplit('~', left, right)) {
            self.Equality.push_back(false);
        } else {
            Y_ENSURE(false, "Invalid format of TAlignment");
        }
        self.Segments1.push_back(FromString<size_t>(left));
        self.Segments2.push_back(FromString<size_t>(right));
    }
    Y_ENSURE(self.IsValid());
    return self;
}

// ~~~~ TAlignmentMap ~~~~

TAlignmentMap::TAlignmentMap(const TVector<size_t>& srcSegments, const TVector<size_t>& destSegments,
    const TVector<bool>& equality)
{
    const size_t count = srcSegments.size();
    Y_ENSURE(srcSegments.size() == count);
    Y_ENSURE(destSegments.size() == count);
    Y_ENSURE(equality.size() == count);

    Map.reserve(count);
    EqualityIntegral.reserve(count + 1);
    EqualityIntegral.push_back(0);

    size_t destBegin = 0;
    i32 equalitySum = 0;
    for (size_t i = 0; i < srcSegments.size(); ++i) {
        const size_t srcSegment = srcSegments[i];
        const size_t destEnd = destBegin + destSegments[i];
        const i32 equalityDelta = equality[i] ? 1 : 0;
        for (size_t j = 0; j < srcSegment; ++j) {
            Map.push_back({destBegin, destEnd});
            equalitySum += equalityDelta;
            EqualityIntegral.push_back(equalitySum);
        }
        destBegin = destEnd;
    }
}

NNlu::TInterval TAlignmentMap::ConvertInterval(const NNlu::TInterval& rawInterval) const {
    const NNlu::TInterval interval = MakeSrcIntervalSafe(rawInterval);
    if (interval.Empty()) {
        size_t pos = 0;
        if (interval.Begin < Map.size()) {
            pos = Map[interval.Begin].Begin;
        } else if (interval.End > 0) {
            pos = Map[interval.End - 1].End;
        }
        return {pos, pos};
    }
    Y_ASSERT(interval.Begin < Map.size());
    Y_ASSERT(interval.End > 0);
    NNlu::TInterval result;
    result.Begin = Map[interval.Begin].Begin;
    result.End = Map[interval.End - 1].End;
    Y_ENSURE(result.Valid());
    return result;
}

NNlu::TInterval TAlignmentMap::MakeSrcIntervalSafe(const NNlu::TInterval& rawInterval) const {
    NNlu::TInterval result = rawInterval;
    result.Begin = Max<size_t>(0, result.Begin);
    result.End = Max(result.Begin, result.End);
    result.Begin = Min(Map.size(), result.Begin);
    result.End = Min(Map.size(), result.End);
    return result;
}

bool TAlignmentMap::HasSureMatch(const NNlu::TInterval& interval) const {
    return GetEqualTokenCount(interval) > 0;
}

bool TAlignmentMap::HasStrictMatch(const NNlu::TInterval& interval) const {
    return GetEqualTokenCount(interval) == interval.Length();
}

bool TAlignmentMap::GetEqualTokenCount(const NNlu::TInterval& rawInterval) const {
    const NNlu::TInterval interval = MakeSrcIntervalSafe(rawInterval);
    return EqualityIntegral[interval.End] - EqualityIntegral[interval.Begin];
}

// ~~~~ TAlignment ~~~~

TAlignment::TAlignment(TAlignmentData&& data)
    : Data(std::move(data))
    , Map1To2(Data.Segments1, Data.Segments2, Data.Equality)
    , Map2To1(Data.Segments2, Data.Segments1, Data.Equality)
{
}

// static
TAlignment TAlignment::CreateTrivial(size_t count) {
    return TAlignment(TAlignmentData::CreateTrivial(count));
}

TString TAlignment::WriteToString() const {
    return Data.WriteToString();
}

// static
TAlignment TAlignment::ReadFromString(TStringBuf str) {
    return TAlignment(TAlignmentData::ReadFromString(str));
}

} // namespace NNlu

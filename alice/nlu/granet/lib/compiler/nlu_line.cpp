#include "nlu_line.h"
#include <alice/nlu/granet/lib/utils/string_utils.h>
#include <util/charset/utf8.h>
#include <util/string/builder.h>
#include <util/stream/format.h>
#include <util/string/join.h>

namespace NGranet {

namespace {

class TParsedStringBuf {
public:
    TStringBuf Str;
    bool IsFail = false;

    TParsedStringBuf(TStringBuf str)
        : Str(str)
    { }

    TStringBuf ExtractTill(char delimiter) {
        return Str.NextTokAt(Str.find(delimiter));
    }

    TStringBuf ExtractBeforeAndSkip(char delimiter) {
        const TStringBuf token = ExtractTill(delimiter);
        SkipPrefix(delimiter);
        return token;
    }

    bool TrySkipPrefix(char prefix) {
        if (!Str.StartsWith(prefix)) {
            return false;
        }
        Str.Skip(1);
        return true;
    }

    void SkipPrefix(char prefix) {
        if (!TrySkipPrefix(prefix)) {
            IsFail = true;
        }
    }
};

} // namespace

bool TryParseNluTemplateLine(TStringBuf original, TVector<TNluLinePart>* parts, TVector<TTag>* tags) {
    Y_ENSURE(parts);
    Y_ENSURE(tags);

    parts->clear();
    tags->clear();

    TParsedStringBuf parsedLine(original);
    parsedLine.Str.SplitOff('#');
    bool isInsideSlot = false;

    while (!parsedLine.Str.empty()) {
        TTag tag;
        tag.Interval.Begin = parts->size();

        TParsedStringBuf parsedTag(parsedLine.ExtractTill('\''));
        while (!parsedTag.Str.empty()) {
            const TStringBuf staticText = StripString(parsedTag.ExtractTill('@'));
            if (!staticText.empty()) {
                parts->push_back({TString(staticText), "", ""});
            }
            if (parsedTag.TrySkipPrefix('@')) {
                const TStringBuf elementName = parsedTag.ExtractBeforeAndSkip('(');
                const TStringBuf elementSuffix = parsedTag.ExtractBeforeAndSkip(')');
                parts->push_back({"", TString(elementName), TString(elementSuffix)});
                parsedTag.IsFail = parsedTag.IsFail || elementName.Contains(' ') || elementName.Contains('\'');
            }
            if (parsedTag.IsFail) {
                parts->clear();
                tags->clear();
                return false;
            }
        }

        tag.Interval.End = parts->size();

        if (isInsideSlot) {
            parsedLine.SkipPrefix('\'');
            parsedLine.SkipPrefix('(');
            tag.Name = parsedLine.ExtractBeforeAndSkip(')');
        } else {
            parsedLine.TrySkipPrefix('\'');
        }
        if (!tag.Interval.Empty()) {
            tags->push_back(tag);
        }
        isInsideSlot = !isInsideSlot;
        if (parsedLine.IsFail) {
            parts->clear();
            tags->clear();
            return false;
        }
    }
    return true;
}

} // namespace NGranet

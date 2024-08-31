#pragma once

#include <alice/nlu/granet/lib/sample/tag.h>
#include <library/cpp/dbg_output/dump.h>

namespace NGranet {

struct TNluLinePart {
    TString StaticText;
    TString ElementName;
    TString ElementSuffix;
};

// Parse of nlu line with templates, i.e. @custom(5)
// [in] line:    Some 'tagged text'(tag_name) with some @custom(5) 'templates'(+tag_name)
// [out] parts:  {{"Some","",""}, {"tagged text","",""}, {"with some","",""}, {"","custom","5"}, {"templates","",""}}
// [out] tags:   {{0,1,""}, {1,2,"tag_name"}, {2,4,""}, {4,5,"+tag_name"}
bool TryParseNluTemplateLine(TStringBuf line, TVector<TNluLinePart>* parts, TVector<TTag>* tags);

} // namespace NGranet

DEFINE_DUMPER(NGranet::TNluLinePart, StaticText, ElementName, ElementSuffix);

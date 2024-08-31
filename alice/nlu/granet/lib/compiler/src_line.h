#pragma once

#include <alice/nlu/granet/lib/utils/string_utils.h>
#include <alice/nlu/granet/lib/utils/text_view.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>
#include <util/string/cast.h>

namespace NGranet::NCompiler {

// ~~~~ TSrcLine ~~~~

class TSrcLine : public TSimpleRefCount<TSrcLine> {
public:
    using TRef = TIntrusivePtr<TSrcLine>;
    using TConstRef = TIntrusiveConstPtr<TSrcLine>;

public:
    TTextView Source;
    TVector<TSrcLine::TRef> Children;

    // Line has a colon at the end.
    // It is possible to be a parent without children. Example:
    //   root:
    //       # some commented rule
    bool IsParent = false;

public:
    explicit TSrcLine(TTextView source);

    TStringBuf Str() const;

    bool IsCompatibilityMode() const;
    void CheckAllowedChildren(const TArrayRef<const TString>& patterns) const;
    void CheckAllowedChildren(const TArrayRef<const TString>& patterns1,
        const TArrayRef<const TString>& patterns2) const;

    const TSrcLine* FindKey(TStringBuf key) const;
    const TSrcLine* FindKey(TStringBuf key, TStringBuf keyAlias) const;
    const TSrcLine* FindValueByKey(TStringBuf key) const;

    bool GetBooleanValueByKey(TStringBuf key, bool def) const;
    ui32 GetUnsignedIntValueByKey(TStringBuf key, ui32 def) const;

    void DumpTree(IOutputStream* log, const TString& indent = "") const;
};

} // namespace NGranet::NCompiler

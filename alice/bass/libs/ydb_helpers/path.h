#pragma once

#include <util/folder/pathsplit.h>
#include <util/generic/string.h>

namespace NYdbHelpers {

using TPath = TString;

namespace NImpl {

inline void Append(TPathSplitUnix& prefix, TStringBuf component) {
    prefix.AppendComponent(component);
}

template <typename... TArgs>
void Append(TPathSplitUnix& prefix, TStringBuf head, const TArgs&... tail) {
    prefix.AppendComponent(head);
    Append(prefix, tail...);
}

} // namespace NImpl

template <typename... TArgs>
TPath Join(const TPath& head, const TArgs&... tail) {
    TPathSplitUnix prefix(head);
    NImpl::Append(prefix, tail...);
    return prefix.Reconstruct();
}

struct TTablePath {
    TTablePath(const TPath& database, const TString& name)
        : Database(database)
        , Name(name) {
    }

    TString FullPath() const {
        return Join(Database, Name);
    }

    TPath Database;
    TString Name;
};

template <typename... TArgs>
TTablePath Join(const TTablePath& path, const TArgs&... tail) {
    return {path.Database, Join(path.Name, tail...)};
}

} // namespace NYdbHelpers

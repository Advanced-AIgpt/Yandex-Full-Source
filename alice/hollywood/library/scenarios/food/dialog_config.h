#pragma once

#include <alice/nlu/granet/lib/utils/flag_utils.h>

#include <util/datetime/base.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>

namespace NAlice::NHollywood::NFood {

// ~~~~ TEventKey ~~~~

struct TEventKey {
    TString GroupName;
    TString FrameName;

    bool IsDefined() const {
        return !FrameName.empty();
    }

    bool IsEqual(TStringBuf groupName, TStringBuf frameName) const {
        // a bit of optimization
        return GroupName.size() == groupName.size()
            && FrameName.size() == frameName.size()
            && GroupName == groupName
            && FrameName == frameName;
    }

    bool operator==(const TEventKey& other) const {
        return IsEqual(other.GroupName, other.FrameName);
    }
};

bool IsFrameClean(const TEventKey& eventKey);

// ~~~~ TFrameGroupConfig ~~~~

struct TFrameGroupConfig {
    TVector<TString> Frames;
    TString FallbackNlg;
};

// ~~~~ EResponseConfigFlag ~~~~

enum EResponseConfigFlag : ui32 {
    RCF_LISTEN       = FLAG32(0),
    RCF_SILENT       = FLAG32(1)
};

Y_DECLARE_FLAGS(EResponseConfigFlags, EResponseConfigFlag);
Y_DECLARE_OPERATORS_FOR_FLAGS(EResponseConfigFlags);

// ~~~~ TSuggest ~~~~

const TVector<TString>& GetNlgHardcodedItems();
bool IsDishSuggest(const TString& suggest);

// ~~~~ TResponseConfig ~~~~

struct TResponseConfig {
    EResponseConfigFlags Flags = 0;
    TVector<TString> ExpectedFrameGroups;
    TVector<TString> Suggests;
    bool UseLastExpectedFrameGroups = false;
};

// ~~~~ TDialogConfig ~~~~

struct TDialogConfig {
    TDuration FallbackTtl;
    TDuration ShortMemoryTtl;
    TDuration PlaceSlugTtl;
    TDuration RequestTtl;
    THashSet<TString> WeakFrames;
    THashMap<TString, TFrameGroupConfig> FrameGroups;
    THashMap<TString, TResponseConfig> Responses;

    bool IsFrameMain(TStringBuf frameName) const;
    bool IsFrameCart(TStringBuf frameName) const;
    bool IsFrameAction(TStringBuf frameName) const;
    bool IsFrameWeak(TStringBuf frameName) const;

    bool IsFrameInGroup(TStringBuf frameName, TStringBuf groupName) const;

    const TFrameGroupConfig& GetFrameGroup(TStringBuf groupName) const;
    const TResponseConfig& GetResponse(TStringBuf name) const;
};

const TDialogConfig& GetDialogConfig();

} // namespace NAlice::NHollywood::NFood

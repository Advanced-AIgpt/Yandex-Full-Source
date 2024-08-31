#pragma once
#include <util/generic/string.h>
#include <util/generic/map.h>
#include <util/generic/hash.h>
#include <util/generic/bitmap.h>
#include <util/generic/yexception.h>
#include <util/str_stl.h>
#include "node.h"
#include "parser.h"

namespace NVoice {

enum class ENodeMode {
    Required,
    Optional
};

class TProfileBuilder {
public:
    TProfileBuilder(TNode::ContainerType& nodeMap, uint16_t& totalCount, TProfileFieldsMask& profileFields, TProfileBuilder* parent = nullptr);

    TProfileBuilder& AddField(TStringBuf key, uint16_t typeMask = NODE_ANY, ENodeMode mode = ENodeMode::Optional);
    TProfileBuilder& RemoveField(TStringBuf key);
    TProfileBuilder BeginSubMap();
    TProfileBuilder& EndSubMap();

private:
    TNode::ContainerType& NodeMap;
    uint16_t& TotalCount;
    TProfileFieldsMask& ProfileFields;
    TProfileBuilder* const Parent;
    TNode* LastAdded;
};


class TParserBuilder {
public:
    TParserBuilder();

    // construct new clean profile
    TProfileBuilder AddNewProfile(const TProfileKey& profileKey = {});

    // construct profile as a copy of existing one
    TProfileBuilder AddCopiedPofile(const TProfileKey& sourceProfileKey, const TProfileKey& copyProfileKey);

    TParser CreateParser();
    THolder<TParser> CreateParserInHeap();

private:
    uint16_t TotalCount = 0;
    TNode RootNode;
    TParser::TProfilesMap Profiles;
};

} // namespace NVoice

#pragma once
#include <cinttypes>
#include <util/generic/ptr.h>
#include <util/generic/map.h>
#include <util/generic/hash.h>

namespace NVoice {

enum ENodeType : uint16_t {
    NODE_MAP        = 1,
    NODE_ARRAY      = 1 << 1,
    NODE_STRING     = 1 << 2,
    NODE_INTEGER    = 1 << 3,
    NODE_FLOAT      = 1 << 4,
    NODE_BOOLEAN    = 1 << 5,
    NODE_NULL       = 1 << 6,
    NODE_ANY        = NODE_MAP|NODE_ARRAY|NODE_STRING|NODE_INTEGER|NODE_FLOAT|NODE_BOOLEAN|NODE_NULL,
    NODE_NUMBER     = NODE_INTEGER|NODE_FLOAT
};

// Represents JSON Map field's value
struct TNode {
    using ContainerType = THashMap<TString, TNode>;

    uint16_t TypeMask = 0;
    uint16_t Idx = UINT16_MAX;
    THolder<ContainerType> SubMap = nullptr;
};


const TNode* GetByPath(const TNode& root, TStringBuf path);

bool Contains(const TNode& root, const TNode& node);

}  // namespace NVoice

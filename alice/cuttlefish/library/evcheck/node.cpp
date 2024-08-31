#include "node.h"

namespace NVoice {

const TNode* GetByPath(const TNode& root, TStringBuf path)
{
    if (root.SubMap == nullptr)
        return nullptr;
    if (const TNode* node = root.SubMap->FindPtr(path))
        return node;

    TStringBuf l, r;
    if (!path.TrySplit('/', l, r))
        return nullptr;
    if (l.empty())
        return GetByPath(root, r);
    if (const TNode* subNode = root.SubMap->FindPtr(l))
        return GetByPath(*subNode, r);

    return nullptr;
}

bool Contains(const TNode& root, const TNode& node)
{
    if (&root == &node)
        return true;
    if (root.SubMap == nullptr)
        return false;

    for (const auto& it : *root.SubMap) {
        if (Contains(it.second, node))
            return true;
    }
    return false;
}

} // namespace NVoice


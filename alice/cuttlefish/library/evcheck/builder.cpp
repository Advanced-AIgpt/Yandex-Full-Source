#include "builder.h"


namespace NVoice {

namespace {

TNode::ContainerType& EnsureSubMap(TNode& node)
{
    if (!node.SubMap)
        node.SubMap = MakeHolder<TNode::ContainerType>();
    return *node.SubMap;
}

}  // anonymous namespace


TProfileBuilder::TProfileBuilder(TNode::ContainerType& nodeMap, uint16_t& totalCount, TProfileFieldsMask& profileFields, TProfileBuilder* parent)
    : NodeMap(nodeMap)
    , TotalCount(totalCount)
    , ProfileFields(profileFields)
    , Parent(parent)
    , LastAdded(nullptr)
{ }

TProfileBuilder& TProfileBuilder::AddField(TStringBuf key, uint16_t typeMask, ENodeMode mode)
{
    auto res = NodeMap.try_emplace(TString(key), TNode{});
    TNode& node = res.first->second;
    if (res.second) {
        Y_ENSURE(TotalCount < (UINT16_MAX - 1)); // UINT16_MAX is reserved
        node.Idx = TotalCount++;
    }

    // NOTE: for now profile can't validate type of a field, so it may be different (from another profile)
    node.TypeMask |= typeMask;

    ProfileFields.AllowedFields.Set(node.Idx);
    if (mode == ENodeMode::Required) {
        ProfileFields.RequiredFields.Set(node.Idx);
    } else {
        ProfileFields.RequiredFields.Reset(node.Idx);
    }

    LastAdded = &node;
    return *this;
}

TProfileBuilder& TProfileBuilder::RemoveField(TStringBuf key)
{
    if (const TNode* node = NodeMap.FindPtr(key)) {
        ProfileFields.AllowedFields.Reset(node->Idx);
        ProfileFields.RequiredFields.Reset(node->Idx);
    }
    LastAdded = nullptr;
    return *this;
}

TProfileBuilder TProfileBuilder::BeginSubMap()
{
    Y_ENSURE(LastAdded != nullptr && (LastAdded->TypeMask & NODE_MAP));
    return TProfileBuilder(EnsureSubMap(*LastAdded), TotalCount, ProfileFields, this);
}

TProfileBuilder& TProfileBuilder::EndSubMap()
{
    Y_ENSURE(Parent != nullptr);
    return *Parent;
}


TParserBuilder::TParserBuilder()
{
    RootNode.TypeMask = NODE_MAP;
    EnsureSubMap(RootNode);
}

TProfileBuilder TParserBuilder::AddNewProfile(const TProfileKey& profileKey)
{
    auto res = Profiles.try_emplace(profileKey);
    Y_ENSURE(res.second, "Profile with such key already exists");
    return TProfileBuilder(*RootNode.SubMap, TotalCount, res.first->second);
}

TProfileBuilder TParserBuilder::AddCopiedPofile(const TProfileKey& sourceProfileKey, const TProfileKey& copyProfileKey)
{
    const TProfileFieldsMask* sourceProfileFields = Profiles.FindPtr(sourceProfileKey);
    Y_ENSURE(sourceProfileFields != nullptr);

    auto res = Profiles.try_emplace(copyProfileKey, *sourceProfileFields);
    Y_ENSURE(res.second, "Profile with such key already exists");
    return TProfileBuilder(*RootNode.SubMap, TotalCount, res.first->second);
}

TParser TParserBuilder::CreateParser()
{
    return TParser(std::move(RootNode), std::move(Profiles));
}

THolder<TParser> TParserBuilder::CreateParserInHeap()
{
    return MakeHolder<TParser>(std::move(RootNode), std::move(Profiles));
}

} // namespace NVoice

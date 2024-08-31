#include "parser.h"
#include "read_handler.h"
#include "sax_callbacks.h"

namespace NVoice {

namespace {

uint16_t GetIdxByPath(const TNode& rootNode, TStringBuf path) {
    if (const TNode* node = GetByPath(rootNode, path)) {
        return node->Idx;
    }
    return UINT16_MAX;
}

bool CheckProfile(const TDynBitMap& parsedFields, const TProfileFieldsMask& profile) {
    return ((parsedFields & profile.RequiredFields) == profile.RequiredFields)
        && (parsedFields - profile.AllowedFields).Empty();
}

} // anonymous namespace


TParser::TParser(TNode&& rootNode, TProfilesMap&& profiles)
    : RootNode(std::move(rootNode))
    , Profiles(std::move(profiles))
    , SpeechkitVersionNodeIdx(GetIdxByPath(RootNode, "event/payload/speechkitVersion"))
    , AppIdNodeIdx(GetIdxByPath(RootNode, "event/payload/vins/application/app_id"))
    , AuthTokenNodeIdx(GetIdxByPath(RootNode, "event/payload/auth_token"))
{ }

bool TParser::ParseJson(TStringBuf rawJson)
{
    ParsedKey.Clear();
    ParsedFields.Clear();

    // TReadJsonFastCallbacksWithCallback<TParser&> callbacks(RootNode, *this);
    // if (!NJson::ReadJsonFast(rawJson, &callbacks))
    //     return false;

    // TODO: store as a member?
    TJsonReadHandlerWithCallback<TParser&> handler(RootNode, *this);

    rapidjson::Reader reader;
    rapidjson::StringStream ss(rawJson.data());
    if (!reader.Parse(ss, handler)) {
        return false;
    }

    if (const TProfileFieldsMask* profile = Profiles.FindPtr(ParsedKey))
        return CheckProfile(ParsedFields, *profile);
    return true;
}

} // namespace NVoice

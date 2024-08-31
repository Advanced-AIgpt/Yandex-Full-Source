#include "serp_helpers.h"

#include <alice/megamind/protos/common/data_source_type.pb.h>
#include <alice/megamind/protos/scenarios/web_search_source.pb.h>

#include <alice/library/proto/proto_struct.h>

#include <util/generic/vector.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood::NSearch {

namespace {

bool CheckSnippetType(const ::google::protobuf::Struct& snippet, const TStringBuf snippetType) {
    const auto& fields = snippet.fields();
    const auto iter = fields.find("type");
    return iter != fields.end() && iter->second.string_value() == snippetType;
}

bool CheckSnippetType(const ::google::protobuf::Value& snippet, const TStringBuf snippetType) {
    return CheckSnippetType(snippet.struct_value(), snippetType);
}

template<typename TListProto>
void AddIfMatched(const TListProto& doc, const TStringBuf type, TVector<NJson::TJsonValue>& found) {
    for (const auto& field : doc) {
        if (CheckSnippetType(field, type)) {
            found.push_back(JsonFromProto(field));
        }
    }
}

void AddIfMatched(const ::google::protobuf::Struct& doc, const TStringBuf type, TVector<NJson::TJsonValue>& found) {
    if (CheckSnippetType(doc, type)) {
        found.push_back(JsonFromProto(doc));
    }
}

void AddIfMatched(const ::google::protobuf::Value* doc, const TStringBuf type, TVector<NJson::TJsonValue>& found) {
    if (!doc) {
        return;
    }
    if (doc->kind_case() == ::google::protobuf::Value::KindCase::kListValue) {
        AddIfMatched(doc->list_value().values(), type, found);
    } else {
        AddIfMatched(doc->struct_value(), type, found);
    }
}

TVector<NJson::TJsonValue> FindSnippets(const ::google::protobuf::Value& doc, const TStringBuf snippetType,
                                        ESnippetSection section)
{
    TVector<NJson::TJsonValue> found;
    if (section & ESS_SNIPPETS_ALL) {
        const auto& fields = doc.struct_value().fields();
        const auto snippetsIt = fields.find("snippets");
        if (snippetsIt != fields.end()) {
            const auto& snippets = snippetsIt->second.struct_value();
            if (section & ESS_SNIPPETS_PRE) {
                const auto* snippet = FindField(snippets, "pre");
                AddIfMatched(snippet, snippetType, found);
            }
            if (section & ESS_SNIPPETS_MAIN) {
                const auto* snippet = FindField(snippets, "main");
                AddIfMatched(snippet, snippetType, found);
            }
            if (section & ESS_SNIPPETS_POST) {
                const auto* snippet = FindField(snippets, "post");
                AddIfMatched(snippet, snippetType, found);
            }
            if (section & ESS_SNIPPETS_FULL) {
                const auto* snippet = FindField(snippets, "full");
                AddIfMatched(snippet, snippetType, found);
            }
        }
    }
    if (section & ESS_CONSTRUCT) {
        const auto* construct = FindField(doc.struct_value(), "construct");
        AddIfMatched(construct, snippetType, found);
    }
    if (section & ESS_FLAT) {
        AddIfMatched(&doc, snippetType, found);
    }
    return found;
}

TVector<NJson::TJsonValue> FindSnippets(const NScenarios::TWebSearchDoc& doc, const TStringBuf snippetType,
                                         ESnippetSection section)
{
    TVector<NJson::TJsonValue> found;
    if (section & ESS_SNIPPETS_ALL) {
        if (section & ESS_SNIPPETS_PRE) {
            AddIfMatched(doc.GetSnippets().GetPre(), snippetType, found);
        }
        if (section & ESS_SNIPPETS_MAIN) {
            AddIfMatched(doc.GetSnippets().GetMain(), snippetType, found);
        }
        if (section & ESS_SNIPPETS_POST) {
            AddIfMatched(doc.GetSnippets().GetPost(), snippetType, found);
        }
        if (section & ESS_SNIPPETS_FULL) {
            AddIfMatched(doc.GetSnippets().GetFull(), snippetType, found);
        }
    }
    if ((section & ESS_CONSTRUCT) && !doc.GetConstruct().empty()) {
        AddIfMatched(doc.GetConstruct(), snippetType, found);
    }
    return found;
}

void MergeJsonValue(const NJson::TJsonValue& patch, NJson::TJsonValue& result) {
    if (result.GetType() != patch.GetType()) {
        return;
    }

    if (result.IsMap() && patch.IsMap()) {
        for (const auto& [key, value] : patch.GetMap()) {
            if (result.Has(key)) {
                MergeJsonValue(value, result[key]);
            } else {
                result.InsertValue(key, value);
            }
        }
    } else if (result.IsArray() && patch.IsArray()) {
        for (const auto& value : patch.GetArray()) {
            result.AppendValue(value);
        }
    } else {
        result = patch;
    }
}

TMaybe<NJson::TJsonValue> FindFactoidInRepeated(const ::google::protobuf::RepeatedPtrField<NScenarios::TWebSearchDoc>& docs,
                                                const TStringBuf snippetType, size_t maxPos, ESnippetSection section, size_t& pos)
{
    for (pos = 0; pos < static_cast<size_t>(docs.size()) && pos < maxPos; ++pos) {
        if (const auto doc = FindSnippet(docs[pos], snippetType, section)) {
            return doc;
        }
    }
    return Nothing();
}

} // namespace

template<typename TDocs>
TMaybe<NJson::TJsonValue> FindSnippet(const TDocs& doc, TStringBuf snippetType, ESnippetSection section) {
    const TVector<NJson::TJsonValue> snippets = FindSnippets(doc, snippetType, section);
    if (snippets.empty()) {
        return Nothing();
    }
    NJson::TJsonValue result(NJson::JSON_MAP);
    for (const auto& snippet : snippets) {
        MergeJsonValue(snippet, result);
    }
    return result;
}

const TSearchResult GetSearchReport(const TScenarioRunRequestWrapper& request) {
    TSearchResult searchResult;

    if (const auto* webSearchDocs = request.GetDataSource(EDataSourceType::WEB_SEARCH_DOCS)) {
        searchResult.DocsLight = &webSearchDocs->GetWebSearchDocs();
    }
    if (const auto* webSearchDocsRight = request.GetDataSource(EDataSourceType::WEB_SEARCH_DOCS_RIGHT)) {
        searchResult.DocsRightLight = &webSearchDocsRight->GetWebSearchDocsRight();
    }
    if (const auto* webSearchWizplaces = request.GetDataSource(EDataSourceType::WEB_SEARCH_WIZPLACES)) {
        searchResult.WizplacesLight = &webSearchWizplaces->GetWebSearchWizplaces();
    }
    if (const auto* webSearchSummarization = request.GetDataSource(EDataSourceType::WEB_SEARCH_SUMMARIZATION)) {
        searchResult.Summarization = &webSearchSummarization->GetWebSearchSummarization();
    }
    if (const auto* webSearchWizard = request.GetDataSource(EDataSourceType::WEB_SEARCH_WIZARD)) {
        searchResult.Wizard = &webSearchWizard->GetWebSearchWizard();
    }
    if (const auto* webSearchBanner = request.GetDataSource(EDataSourceType::WEB_SEARCH_BANNER)) {
        searchResult.Banner = &webSearchBanner->GetWebSearchBanner();
    }
    if (const auto* webSearchRenderrer = request.GetDataSource(EDataSourceType::WEB_SEARCH_RENDERRER); webSearchRenderrer && !webSearchRenderrer->GetWebSearchRenderrer().GetResponse().empty()) {
        searchResult.RenderrerResponse = JsonFromString(webSearchRenderrer->GetWebSearchRenderrer().GetResponse());
    }
    return searchResult;
}

TMaybe<NJson::TJsonValue> FindFactoidInWizplacesImportant(const ::google::protobuf::Struct& wizplaces,
                                                          const TStringBuf factoidType)
{
    for (const auto& [position, wizards] : wizplaces.fields()) {
        if (position != "important") {
            continue;
        }

        for (const auto& wizard : wizards.list_value().values()) {
            for (const auto& [key, value] : wizard.struct_value().fields()) {
                if (key != "construct") {
                    continue;
                }
                for (const auto& block : value.list_value().values()) {
                    const auto& fields = block.struct_value().fields();
                    const auto iter = fields.find("type");
                    if (iter != fields.end() && iter->second.string_value() == factoidType) {
                        return JsonFromProto(block);
                    }
                }
            }
        }
    }
    return Nothing();
}

TMaybe<NJson::TJsonValue> FindFactoidInWizplacesImportant(const NScenarios::TWebSearchWizplaces& wizplaces,
                                                          const TStringBuf factoidType)
{
    for (const auto& wizard : wizplaces.GetImportant()) {
        for (const auto& block : wizard.GetConstruct()) {
            if (CheckSnippetType(block, factoidType)) {
                return JsonFromProto(block);
            }
        }
    }
    return Nothing();
}

TMaybe<NJson::TJsonValue> GetFirstWizplacesImportantConstruct(const ::google::protobuf::Struct& wizplaces) {
    const auto* wizards = FindField(wizplaces, "important");
    if (!wizards || wizards->list_value().values().empty()) {
        return Nothing();
    }
    const auto& wizard = wizards->list_value().values()[0].struct_value();
    const auto* construct = FindField(wizard, "construct");
    if (construct) {
        return JsonFromProto(construct->list_value().values()[0]);
    } else {
        return Nothing();
    }
}

const ::google::protobuf::Value* FindField(const ::google::protobuf::Struct& doc, const TString& name) {
    const auto& fields = doc.fields();
    const auto it = fields.find(name);
    if (it == fields.end()) {
        return nullptr;
    }
    return &(it->second);
}

TMaybe<NJson::TJsonValue> FindFactoidInDocs(const ::google::protobuf::ListValue& docs, const TStringBuf snippetType,
                                            size_t maxPos, ESnippetSection section, size_t& pos)
{
    for (pos = 0; pos < static_cast<size_t>(docs.values().size()) && pos < maxPos; ++pos) {
        if (const auto doc = FindSnippet(docs.values()[pos], snippetType, section)) {
            return doc;
        }
    }
    return Nothing();
}

TMaybe<NJson::TJsonValue> FindFactoidInDocs(const NScenarios::TWebSearchDocs& docs, const TStringBuf snippetType,
                                                 size_t maxPos, ESnippetSection section, size_t& pos)
{
    return FindFactoidInRepeated(docs.GetDocs(), snippetType, maxPos, section, pos);
}

TMaybe<NJson::TJsonValue> FindFactoidInDocs(const NScenarios::TWebSearchDocsRight& docs, const TStringBuf snippetType,
                                                 size_t maxPos, ESnippetSection section, size_t& pos)
{
    return FindFactoidInRepeated(docs.GetDocsRight(), snippetType, maxPos, section, pos);
}

/**********************************************************************************************
 * New search processing
 **********************************************************************************************/
/*
    Find entry in search results
    @param context - previously filled context
    @param where - where need to search
    @param path - initial path to search data (with alice/library/proto_struct enumeration syntax)
    @param keyCondition - subkey to check a value (usually, "type"). Empty() to skip check
    @param value - value of subkey (i.e. "entity_search", etc)

    @return pointer to found structure or nullptr
*/
const google::protobuf::Struct* FindEntry(const TSearchResult& context,
                                          ESearchLocation where,
                                          const TString& path,
                                          const TString& keyCondition,
                                          const TString& value,
                                          const TProtoStructParser& psp /*= TProtoStructParser{}*/)
{
    const google::protobuf::Struct* result = nullptr;
    auto fn = [&result, keyCondition, value, psp](const google::protobuf::Struct& obj) -> bool {
        if (keyCondition.Empty() || psp.GetValueString(obj, keyCondition, "") == value) {
            result = &obj;
            return true;
        }
        return false;
    };

    switch (where) {
        case ESearchLocation::Docs:
            if (context.Docs == nullptr) {
                return nullptr;
            }
            for (const auto& it : context.Docs->values()) {
                if (it.has_struct_value()) {
                    psp.EnumerateKeys(it.struct_value(), path, fn);
                }
            }
            break;
        case ESearchLocation::DocsRight:
            if (context.DocsRight == nullptr) {
                return nullptr;
            }
            for (const auto& it : context.DocsRight->values()) {
                if (it.has_struct_value()) {
                    psp.EnumerateKeys(it.struct_value(), path, fn);
                }
            }
            break;
        case ESearchLocation::DocsLight:
            if (context.DocsLight == nullptr) {
                return nullptr;
            }
            for (const auto& it : context.DocsLight->GetDocs()) {
                // TODO [DD] Pre/Post/Main/Full
                psp.EnumerateKeys(it.GetSnippets().GetFull(), path, fn);
            }
            break;
        case ESearchLocation::DocsRightLight:
            if (context.DocsRightLight == nullptr) {
                return nullptr;
            }
            for (const auto& it : context.DocsRightLight->GetDocsRight()) {
                // TODO [DD] Pre/Post/Main/Full
                psp.EnumerateKeys(it.GetSnippets().GetFull(), path, fn);
            }
            break;
        case ESearchLocation::Wizplaces:
            if (context.Wizplaces == nullptr) {
                return nullptr;
            }
            psp.EnumerateKeys(*context.Wizplaces, path, fn);
            break;
        case ESearchLocation::WizplacesLight:
            if (context.WizplacesLight == nullptr) {
                return nullptr;
            }
            for (const auto& it : context.WizplacesLight->GetImportant()) {
                psp.EnumerateKeys(it.GetSnippets().GetMain(), path, fn);
            }
            break;
    }
    return result;
}

} // namespace NAlice::NHollywood::NSearch

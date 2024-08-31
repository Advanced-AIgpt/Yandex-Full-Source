#pragma once

#include <alice/hollywood/library/request/request.h>

#include <alice/megamind/protos/scenarios/request.pb.h>
#include <alice/library/json/json.h>
#include <alice/library/proto/proto_struct.h>

#include <util/generic/string.h>
#include <util/generic/maybe.h>
#include <util/generic/vector.h>

namespace NAlice::NHollywood::NSearch {

struct TSearchResult {
    const ::google::protobuf::Struct* Wizplaces = nullptr;
    const ::google::protobuf::ListValue* Docs = nullptr;
    const ::google::protobuf::ListValue* DocsRight = nullptr;
    const NScenarios::TWebSearchDocs* DocsLight = nullptr;
    const NScenarios::TWebSearchDocsRight* DocsRightLight = nullptr;
    const NScenarios::TWebSearchWizplaces* WizplacesLight = nullptr;
    const NScenarios::TWebSearchSummarization* Summarization = nullptr;
    const NScenarios::TWebSearchBanner* Banner = nullptr;
    const NScenarios::TWebSearchWizard* Wizard = nullptr;
    NJson::TJsonValue RenderrerResponse;
};

const TSearchResult GetSearchReport(const TScenarioRunRequestWrapper& request);

//
// Old search requests, based on JSON answer
//
TMaybe<NJson::TJsonValue> FindFactoidInWizplacesImportant(const ::google::protobuf::Struct& wizplaces,
                                                          const TStringBuf factoidType);

TMaybe<NJson::TJsonValue> FindFactoidInWizplacesImportant(const NScenarios::TWebSearchWizplaces& wizplaces,
                                                          const TStringBuf factoidType);

TMaybe<NJson::TJsonValue> GetFirstWizplacesImportantConstruct(const ::google::protobuf::Struct& wizplaces);

enum ESnippetSection {
    ESS_NONE = 0,
    ESS_SNIPPETS_PRE = 1,
    ESS_SNIPPETS_MAIN = 2,
    ESS_SNIPPETS_POST = 4,
    ESS_SNIPPETS_FULL = 8,
    ESS_CONSTRUCT = 16,
    ESS_FLAT = 32,
    ESS_SNIPPETS_ALL = ESS_SNIPPETS_PRE | ESS_SNIPPETS_MAIN | ESS_SNIPPETS_POST | ESS_SNIPPETS_FULL,
    ESS_ALL = ESS_SNIPPETS_ALL | ESS_CONSTRUCT | ESS_FLAT
};

template<typename TDocs>
TMaybe<NJson::TJsonValue> FindSnippet(const TDocs& doc, TStringBuf snippetType, ESnippetSection section);

const ::google::protobuf::Value* FindField(const ::google::protobuf::Struct& doc, const TString& name);

TMaybe<NJson::TJsonValue> FindFactoidInDocs(const ::google::protobuf::ListValue& docs, const TStringBuf snippetType,
                                            size_t maxPos, ESnippetSection section, size_t& pos);

TMaybe<NJson::TJsonValue> FindFactoidInDocs(const NScenarios::TWebSearchDocs& docs, const TStringBuf snippetType,
                                            size_t maxPos, ESnippetSection section, size_t& pos);

TMaybe<NJson::TJsonValue> FindFactoidInDocs(const NScenarios::TWebSearchDocsRight& docs, const TStringBuf snippetType,
                                            size_t maxPos, ESnippetSection section, size_t& pos);

//
// New search requests, based on proto_struct parsers
//

enum class ESearchLocation {
    Docs,
    DocsRight,
    DocsLight,
    DocsRightLight,
    Wizplaces,
    WizplacesLight,
};

const google::protobuf::Struct* FindEntry(const TSearchResult& context,
                                          ESearchLocation where,
                                          const TString& path,
                                          const TString& keyCondition,
                                          const TString& value,
                                          const TProtoStructParser& psp = TProtoStructParser{});

} // namespace NAlice::NHollywood::NSearch

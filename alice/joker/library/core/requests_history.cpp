#include "requests_history.h"

#include <library/cpp/json/json_writer.h>

#include <util/charset/utf8.h>

namespace NAlice::NJoker {

namespace {

// Headers are case-insensitive, so make them lowercase
const TVector<TStringBuf> IGNORED_HEADERS_STARTS_WITH = {
    TStringBuf("authorization"),
    TStringBuf("x-yandex-proxy-header"),
    TStringBuf("x-rtlog-token"),
    TStringBuf("x-yandex-fake-time"),
    TStringBuf("x-yandex-joker"),
    TStringBuf("x-yandex-via-proxy")
};

TRequestsHistory::TRequestEntry RequestEntryFromHttpContext(const IHttpContext& ctx) {
    TRequestsHistory::TRequestEntry entry;

    // Action (from URI)
    entry.Action = ctx.Uri().PrintS(NUri::TField::EFlags::FlagAction);

    // Query (from URI)
    bool isCgiReadFromBody = false;
    TCgiParameters cgi = IHttpContext::ConstructCgi(ctx.Headers(), ctx.Uri(), ctx.Body(), isCgiReadFromBody);
    for (const auto& pair : cgi) {
        entry.Query[pair.first] = pair.second;
    }

    // Headers
    for (const auto& header : ctx.Headers()) {
        auto name = ToLowerUTF8(header.Name());
        bool isIgnored = AnyOf(IGNORED_HEADERS_STARTS_WITH.begin(), IGNORED_HEADERS_STARTS_WITH.end(), [&name](const auto& h) {
            return name.StartsWith(h);
        });
        if (!isIgnored) {
            entry.Headers[name] = header.Value();
        }
    }

    // Body
    if (!isCgiReadFromBody) {
        auto contentTypeHeader = entry.Headers.find("content-type");
        if (contentTypeHeader != entry.Headers.end() && AsciiEqualsIgnoreCase(contentTypeHeader->second, "application/protobuf")) {
            // need to be saved to an ASCII-readable format
            entry.Body = TStringBuilder{} << "<protobuf of size " << ctx.Body().Size() << ">";
        } else {
            entry.Body = ctx.Body();
        }
    }

    return entry;
}

} // namespace

TRequestsHistory::TRequestsHistory(size_t maxSize)
    : Storage_{maxSize}
{
}

void TRequestsHistory::Add(const TString& groupId, const IHttpContext& ctx) {
    TRequestEntry entry = RequestEntryFromHttpContext(ctx);
    auto value = MakeIntrusive<TRequestEntry>(std::move(entry));

    {
        TWriteGuard g(Lock_);

        auto iter = Storage_.Find(groupId);
        if (iter == Storage_.End()) {
            Storage_.Insert(groupId, {value});
        } else {
            iter->emplace_back(value);
        }
    }
}

TMaybe<TRequestsHistory::TRequestEntries> TRequestsHistory::Get(const TString& groupId) {
    TReadGuard g(Lock_);

    TRequestEntries entries;
    if (Storage_.PickOut(groupId, &entries)) {
        return entries;
    }
    return Nothing();
}

// static
TString TRequestsHistory::ToJson(const TRequestsHistory::TRequestEntries& entries) {
    NJson::TJsonValue entriesJson(NJson::JSON_ARRAY);

    for (const auto& entry : entries) {
        NJson::TJsonValue entryJson(NJson::JSON_MAP);

        // Action
        if (!entry->Action.Empty()) {
            entryJson["action"] = entry->Action;
        }

        // Query
        if (!entry->Query.empty()) {
            NJson::TJsonValue queryJson(NJson::JSON_MAP);
            for (const auto& pair : entry->Query) {
                queryJson[pair.first] = pair.second;
            }
            entryJson["query"] = queryJson;
        }

        // Headers
        if (!entry->Headers.empty()) {
            NJson::TJsonValue headersJson(NJson::JSON_MAP);
            for (const auto& pair : entry->Headers) {
                headersJson[pair.first] = pair.second;
            }
            entryJson["headers"] = headersJson;
        }

        // Body
        if (!entry->Body.Empty()) {
            entryJson["body"] = entry->Body;
        }

        entriesJson.AppendValue(entryJson);
    }

    return NJson::WriteJson(entriesJson, /* formatOutput = */ true, /* sortKeys = */ true);
}

} // namespace NJoker

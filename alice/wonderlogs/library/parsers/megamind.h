#pragma once

#include <alice/wonderlogs/protos/megamind_prepared.pb.h>
#include <alice/wonderlogs/protos/uniproxy_prepared.pb.h>

#include <library/cpp/yson/node/node.h>

#include <util/generic/maybe.h>

namespace NAlice::NWonderlogs {

struct TMegamindParsedLogs {
    TMaybe<TMegamindPrepared::TMegamindRequestResponse> RequestResponse;
    TVector<TMegamindPrepared::TError> Errors;
};

namespace NTestSuiteMegamind {

struct TTestCaseParseUuid;

} // namespace NTestSuiteMegamind

class TMegamindLogsParser {
public:
    explicit TMegamindLogsParser(const NYT::TNode& row);
    TMegamindParsedLogs Parse() const;

private:
    friend struct NAlice::NWonderlogs::NTestSuiteMegamind::TTestCaseParseUuid;

    static TMaybe<TString> ParseUuid(const NYT::TNode& row);
    static TMaybe<TString> ParseRequestId(const NYT::TNode& row);
    static TMaybe<TString> ParseResponseId(const NYT::TNode& row);
    static TMaybe<TString> ParseMessageId(const NYT::TNode& row);

    static TMegamindPrepared::TError GenerateError(const TMegamindPrepared::TError::EReason reason,
                                                   const TString& message, const TMaybe<TString>& uuid,
                                                   const TMaybe<TString>& requestId, const TMaybe<TString>& messageId);

    const NYT::TNode& Row;
};

} // namespace NAlice::NWonderlogs

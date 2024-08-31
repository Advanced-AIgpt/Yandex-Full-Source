#pragma once

#include "answers.h"
#include "common_headers.h"

#include <alice/bass/libs/fetcher/request.h>

#include <library/cpp/cgiparam/cgiparam.h>

namespace NBASS::NMusic {

constexpr TStringBuf TYPE_FIELD = "type";
constexpr TStringBuf PATH_FIELD = "path";
constexpr TStringBuf ID_FIELD = "id";
constexpr TStringBuf USER_ID_FIELD = "userId";
constexpr TStringBuf KIND_FIELD = "kind";
constexpr TStringBuf BULK_DATA_FIELD = "bulk_data";
constexpr TStringBuf RICH_TRACKS_FIELD = "richTracks";
constexpr TStringBuf REQUEST_FIELD = "request";
constexpr TStringBuf RESPONSE_FIELD = "response";
constexpr TStringBuf RESULT_FIELD = "result";

class TMusicCatalog {
public:
    TMusicCatalog(const TCommonHeaders& commonHeaders,
                  const NAlice::TClientFeatures& clientFeatures,
                  bool autoplay,
                  bool shouldLogResponse);

    virtual ~TMusicCatalog() = default;

    bool ProcessRequest(NSc::TValue* out);

    int GetHttpCode() const {
        return HttpCode;
    }

protected:
    virtual bool ValidateCatalogAnswer(const NSc::TValue& musicAnswer, NSc::TValue* out) const = 0;

    bool ValidateSingleCatalogAnswer(const NSc::TValue& musicAnswer, TStringBuf answerType, NSc::TValue* out) const;

protected:
    const TCommonHeaders& CommonHeaders;
    const NAlice::TClientFeatures& ClientFeatures;
    bool Autoplay;
    bool ShouldLogResponse;
    NHttpFetcher::THandle::TRef Handler;
    int HttpCode = 0;
};

class TMusicCatalogSingle final : public TMusicCatalog {
public:
    TMusicCatalogSingle(const TCommonHeaders& commonHeaders,
                        const NAlice::TClientFeatures& clientFeatures,
                        TStringBuf answerType,
                        bool autoplay,
                        bool shouldLogResponse);

    void CreateRequestHandler(NHttpFetcher::TRequestPtr request, const TCgiParameters& cgi, NSc::TValue* data = nullptr);

private:
    bool ValidateCatalogAnswer(const NSc::TValue& musicAnswer, NSc::TValue* out) const override;

private:
    TString AnswerType;
};

class TMusicCatalogBulk final : public TMusicCatalog {
public:
    using TMusicCatalog::TMusicCatalog;

    void CreateRequestHandler(NHttpFetcher::TRequestPtr request, const NSc::TValue& body);

private:
    bool ValidateCatalogAnswer(const NSc::TValue& musicAnswer, NSc::TValue* out) const override;
};

} // namespace NBASS::NMusic

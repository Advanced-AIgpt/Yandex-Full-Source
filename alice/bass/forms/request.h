#pragma once

#include "../http_request.h"

#include "../../../util/generic/ptr.h"

class IThreadPool;

namespace NBASS {

inline constexpr TStringBuf APPLY_MODE = "apply";
inline constexpr TStringBuf COMMIT_MODE = "commit";

/** Base class for all requests from vins (setup/report).
 */
class TBassRequestHandler : public TJsonHttpRequestHandler {
public:
    static void RegisterHttpHandlers(THttpHandlersMap* handlers, TGlobalContextPtr globalCtx);

protected:
    // Overriden from TJsonHttpRequestHandler
    const TString& GetReqIdClass() const override;
};

/** Http /setup handler registrator.
 */
class TSetupBassRequestHandler final : public TBassRequestHandler {
public:
    explicit TSetupBassRequestHandler(TGlobalContextPtr globalCtx);

    // Overriden from TJsonHttpRequestHandler
    HttpCodes DoJsonReply(TGlobalContextPtr globalCtx, const NSc::TValue& request,
                          const TParsedHttpFull& httpRequest, const THttpHeaders& httpHeaders,
                          NSc::TValue* response) override;

private:
    THolder<IThreadPool> ThreadPool;
};

/** Http /vins handler registrator.
 */
class TReportBassRequestHandler final : public TBassRequestHandler {
public:
    // Overriden from TJsonHttpRequestHandler
    HttpCodes DoJsonReply(NBASS::TGlobalContextPtr globalCtx, const NSc::TValue& request,
                          const TParsedHttpFull& httpRequest, const THttpHeaders& httpHeaders,
                          NSc::TValue* response) override;
};

class TMegamindPrepareRequestHandler final : public TBassRequestHandler {
public:
    // Overriden from TBassRequestHandler
    HttpCodes DoJsonReply(NBASS::TGlobalContextPtr globalCtx, const NSc::TValue& request,
                          const TParsedHttpFull& httpRequest, const THttpHeaders& httpHeaders,
                          NSc::TValue* response) override;
};

class TMegamindApplyRequestHandler final : public TJsonHttpRequestHandler {
public:
    // Overriden from TBassRequestHandler
    HttpCodes DoJsonReply(NBASS::TGlobalContextPtr globalCtx, const NSc::TValue& request,
                          const TParsedHttpFull& httpRequest, const THttpHeaders& httpHeaders,
                          NSc::TValue* response) override;

protected:
    // Overriden from TJsonHttpRequestHandler
    const TString& GetReqIdClass() const override;
};

class TMegamindProtocolRunRequestHandler final : public TMegamindProtocolHttpRequestHandler {
public:
    HttpCodes DoJsonReply(NBASS::TGlobalContextPtr globalCtx, const TString& request,
                          const TParsedHttpFull& httpRequest, const THttpHeaders& httpHeaders,
                          TString& response) override;

protected:
    const TString& GetReqIdClass() const override;
};

/** A base class for protocol commit and apply request handlers */
class TMegamindProtocolCompletionRequestHandler : public TMegamindProtocolHttpRequestHandler {
public:
   explicit TMegamindProtocolCompletionRequestHandler(TStringBuf actionName)
        : ActionName{actionName}
    {
    }
    HttpCodes DoJsonReply(NBASS::TGlobalContextPtr globalCtx, const TString& request,
                          const TParsedHttpFull& httpRequest, const THttpHeaders& httpHeaders,
                          TString& response) override;
protected:
    virtual HttpCodes Process(const TString& authHeader, const TString& appInfoHeader,
                              const TMaybe<TString>& userTicketHeader, TGlobalContextPtr globalCtx,
                              const NSc::TValue& input, NSc::TValue& output, const NSc::TValue& configPatch) const = 0;

private:
    TString ActionName;
};

class TMegamindProtocolCommitRequestHandler final : public TMegamindProtocolCompletionRequestHandler {
public:
    TMegamindProtocolCommitRequestHandler()
        : TMegamindProtocolCompletionRequestHandler{COMMIT_MODE}
    {
    }

protected:
    HttpCodes Process(const TString& authHeader, const TString& appInfoHeader,
                      const TMaybe<TString>& userTicketHeader, TGlobalContextPtr globalCtx,
                      const NSc::TValue& input, NSc::TValue& output, const NSc::TValue& configPatch) const override;
    const TString& GetReqIdClass() const override;
};

class TMegamindProtocolApplyRequestHandler final : public TMegamindProtocolCompletionRequestHandler {
public:
    TMegamindProtocolApplyRequestHandler()
        : TMegamindProtocolCompletionRequestHandler{APPLY_MODE}
    {
    }

protected:
    HttpCodes Process(const TString& authHeader, const TString& appInfoHeader,
                      const TMaybe<TString>& userTicketHeader, TGlobalContextPtr globalCtx, const NSc::TValue& input,
                      NSc::TValue& output, const NSc::TValue& configPatch) const override;
    const TString& GetReqIdClass() const override;
};

} // namespace NBASS

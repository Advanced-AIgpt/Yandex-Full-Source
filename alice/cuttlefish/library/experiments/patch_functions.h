#pragma once
#include "experiment_patch.h"
#include "session_context_proxy.h"
#include <library/cpp/json/json_value.h>
#include <library/cpp/regex/pcre/regexp.h>
#include <util/generic/set.h>


namespace NVoice::NExperiments {
// ------------------------------------------------------------------------------------------------

class TIfEventType : public IPatchFunction {
public:
    TIfEventType(const TString& eventType);
    bool Apply(NJson::TJsonValue& event, const NAliceProtocol::TSessionContext&) const override;

private:
    TString EventType;
};

// ------------------------------------------------------------------------------------------------
class TSet : public IPatchFunction {
public:
    TSet(const TString& path, const NJson::TJsonValue& value);
    bool Apply(NJson::TJsonValue& event, const NAliceProtocol::TSessionContext&) const override;

protected:
    TVector<TString> Path;
    NJson::TJsonValue Value;
    bool Overwrite = true;
};

// ------------------------------------------------------------------------------------------------
class TSetIfNone : public TSet {
public:
    TSetIfNone(const TString& path, const NJson::TJsonValue& value);
};

// ------------------------------------------------------------------------------------------------
class TAppend : public IPatchFunction {
public:
    TAppend(const TString& path, const NJson::TJsonValue& value);
    bool Apply(NJson::TJsonValue& event, const NAliceProtocol::TSessionContext&) const override;

private:
    TVector<TString> Path;
    NJson::TJsonValue Value;
    bool Tricky = false;
};

// ------------------------------------------------------------------------------------------------
class TExtend : public IPatchFunction {
public:
    TExtend(const TString& path, const NJson::TJsonValue& value);
    bool Apply(NJson::TJsonValue& event, const NAliceProtocol::TSessionContext&) const override;

private:
    TVector<TString> Path;
    NJson::TJsonValue Value;
    bool Tricky = false;
};

// ------------------------------------------------------------------------------------------------
class TDel : public IPatchFunction {
public:
    TDel(const TString& path);
    bool Apply(NJson::TJsonValue& event, const NAliceProtocol::TSessionContext&) const override;

private:
    TVector<TString> Path;
};

// ------------------------------------------------------------------------------------------------
class TImportMacro : public TExtend {
public:
    TImportMacro(const TExpContext& expContext, const TString& path, const TString& macroKey);
};

// ------------------------------------------------------------------------------------------------
class TIfHasStaffLogin : public IPatchFunction {
public:
    TIfHasStaffLogin() = default;
    bool Apply(NJson::TJsonValue& event, const NAliceProtocol::TSessionContext&) const override;
};

// ------------------------------------------------------------------------------------------------
class TIfStaffLoginEq : public IPatchFunction {
public:
    TIfStaffLoginEq(const TString& login);
    bool Apply(NJson::TJsonValue& event, const NAliceProtocol::TSessionContext&) const override;

private:
    const TString StaffLogin;
};

// ------------------------------------------------------------------------------------------------
class TIfStaffLoginIn : public IPatchFunction {
public:
    TIfStaffLoginIn(const NJson::TJsonValue::TArray& logins);
    bool Apply(NJson::TJsonValue&, const NAliceProtocol::TSessionContext&) const override;

private:
    ::TSet<TString> StaffLogins;
};

// ------------------------------------------------------------------------------------------------
class TIfHasPayload : public IPatchFunction {
public:
    TIfHasPayload(const TString& path);
    bool Apply(NJson::TJsonValue&, const NAliceProtocol::TSessionContext&) const override;

private:
    const TSessionContextGetter SessionContextGetter;
    const TVector<TString> Path;
};

// ------------------------------------------------------------------------------------------------
class TIfPayloadEq : public IPatchFunction {
public:
    TIfPayloadEq(const TString& path, const NJson::TJsonValue& value);
    bool Apply(NJson::TJsonValue&, const NAliceProtocol::TSessionContext&) const override;

private:
    const TSessionContextGetter SessionContextGetter;
    const TVector<TString> Path;
    const NJson::TJsonValue Value;
};

// ------------------------------------------------------------------------------------------------
class TIfPayloadNe : public IPatchFunction {
public:
    TIfPayloadNe(const TString& path, const NJson::TJsonValue& value);
    bool Apply(NJson::TJsonValue&, const NAliceProtocol::TSessionContext&) const override;

private:
    const TSessionContextGetter SessionContextGetter;
    const TVector<TString> Path;
    const NJson::TJsonValue Value;
};

// ------------------------------------------------------------------------------------------------
class TIfPayloadIn : public IPatchFunction {
public:
    TIfPayloadIn(const TString& path, const NJson::TJsonValue::TArray& values);
    bool Apply(NJson::TJsonValue&, const NAliceProtocol::TSessionContext&) const override;

private:
    const TSessionContextGetter SessionContextGetter;
    const TVector<TString> Path;
    const NJson::TJsonValue::TArray Values;
};

// ------------------------------------------------------------------------------------------------
class TIfPayloadLike : public IPatchFunction {
public:
    TIfPayloadLike(const TString& path, const TString& regExpr);
    bool Apply(NJson::TJsonValue&, const NAliceProtocol::TSessionContext&) const override;

private:
    const TSessionContextGetter SessionContextGetter;
    const TVector<TString> Path;
    const TRegExMatch Regex;
};

// ------------------------------------------------------------------------------------------------
class TIfHasSessionData : public IPatchFunction {
public:
    TIfHasSessionData(const TString& path);
    bool Apply(NJson::TJsonValue&, const NAliceProtocol::TSessionContext&) const override;

private:
    const TSessionContextGetter SessionContextGetter;
    const TVector<TString> Path;
};

// ------------------------------------------------------------------------------------------------
class TIfSessionDataEq : public IPatchFunction {
public:
    TIfSessionDataEq(const TString& path, const NJson::TJsonValue& value);
    bool Apply(NJson::TJsonValue&, const NAliceProtocol::TSessionContext&) const override;

private:
    const TSessionContextGetter SessionContextGetter;
    const TVector<TString> Path;
    const NJson::TJsonValue Value;
};

// ------------------------------------------------------------------------------------------------
class TIfSessionDataNe : public IPatchFunction {
public:
    TIfSessionDataNe(const TString& path, const NJson::TJsonValue& value);
    bool Apply(NJson::TJsonValue&, const NAliceProtocol::TSessionContext&) const override;

private:
    const TSessionContextGetter SessionContextGetter;
    const TVector<TString> Path;
    const NJson::TJsonValue Value;
};

// ------------------------------------------------------------------------------------------------
class TIfSessionDataIn : public IPatchFunction {
public:
    TIfSessionDataIn(const TString& path, const NJson::TJsonValue::TArray& values);
    bool Apply(NJson::TJsonValue&, const NAliceProtocol::TSessionContext&) const override;

private:
    const TSessionContextGetter SessionContextGetter;
    const TVector<TString> Path;
    const NJson::TJsonValue::TArray Values;
};

// ------------------------------------------------------------------------------------------------
class TIfSessionDataLike : public IPatchFunction {
public:
    TIfSessionDataLike(const TString& path, const TString& regExpr);
    bool Apply(NJson::TJsonValue&, const NAliceProtocol::TSessionContext&) const override;

private:
    const TSessionContextGetter SessionContextGetter;
    const TVector<TString> Path;
    const TRegExMatch Regex;
};

// ------------------------------------------------------------------------------------------------

// Analogue of UniSystem.amend_event_payload
NJson::TJsonValue::TArray ExtractUaasTestsFromMegamindCookie(const NJson::TJsonValue& request);
bool TransferUaasTestsFromMegamindCookie(NJson::TJsonValue& request);

} // namespace NVoice::NExperiments

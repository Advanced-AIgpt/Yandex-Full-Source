#include "validator.h"

#include "error.h"

#include <alice/bass/forms/context/context.h>

#include <util/charset/utf8.h>
#include <util/string/util.h>

namespace NBASS {
namespace NExternalSkill {

NSc::TValue& TSkillValidateHelper::Data() {
    if (!Error) {
        Error.ConstructInPlace(TError::EType::SKILLSERROR, "skill scheme validation error");
    }

    return Error->Data;
}

void TSkillValidateHelper::operator ()(TStringBuf key, TStringBuf errmsg, NDomSchemeRuntime::TValidateInfo vi) {
    if (NDomSchemeRuntime::TValidateInfo::ESeverity::Warning == vi.Severity) {
        return;
    }

    NSc::TValue& jsonDescr = Data()["problems"].Push();
    jsonDescr["type"].SetString(ERRTYPE_UNKNOWN);
    jsonDescr["path"].SetString(key);
    if (IsInternal) {
        jsonDescr["msg"].SetString(errmsg);
    }
}

bool TSkillValidateHelper::CheckForUrl(TStringBuf path, NDomSchemeRuntime::TConstPrimitive<TSchemeTraits, typename TSchemeTraits::TStringType> url) {
    if (url.IsNull()) {
        return true;
    }

    NUri::TUri uri;
    NUri::TState::EParsed uriResult = uri.Parse(url, NUri::TFeature::FeaturesDefault | NUri::TFeature::FeatureSchemeKnown | NUri::TFeature::FeatureAuthSupported);
    if (uriResult == NUri::TState::EParsed::ParsedOK) {
        if (uri.GetScheme() == NUri::TScheme::EKind::SchemeHTTP || uri.GetScheme() == NUri::TScheme::EKind::SchemeHTTPS) {
            return true;
        }
        else {
            NSc::TValue& jsonDescr = Data()["problems"].Push();
            jsonDescr["type"].SetString(ERRTYPE_BADURL);
            jsonDescr["path"].SetString(path);
            if (IsInternal) {
                jsonDescr["msg"].SetString("Unsupported scheme");
            }
            return false;
        }
    }

    NSc::TValue& jsonDescr = Data()["problems"].Push();
    jsonDescr["type"].SetString(ERRTYPE_BADURL);
    jsonDescr["path"].SetString(path);
    if (IsInternal) {
        jsonDescr["msg"].SetString(ParsedStateToString(uriResult));
    }

    return false;
}

bool TSkillValidateHelper::CheckForNotNull(TStringBuf path, const NSc::TValue& value) {
    if (value.IsNull()) {
        NSc::TValue& jsonDescr = Data()["problems"].Push();
        jsonDescr["type"].SetString(ERRTYPE_NULL);
        jsonDescr["path"].SetString(path);
        return false;
    }

    return true;
}

void TSkillValidateHelper::AddApiError(ui32 code, TStringBuf msg, TStringBuf path) {
    NSc::TValue& jsonDescr = Data()["problems"].Push();
    jsonDescr["type"].SetString(ERRTYPE_APIERROR);
    jsonDescr["path"]["code"] = code;
    jsonDescr["path"]["msg"].SetString(msg);
    if (path) {
        jsonDescr["path"]["path"].SetString(path);
    }
}

bool TSkillValidateHelper::CheckForUtf8Size(TStringBuf path, NDomSchemeRuntime::TConstPrimitive<TSchemeTraits, typename TSchemeTraits::TStringType> value, ssize_t from, ssize_t to) {
    TMaybe<NSc::TValue> jsonDescr;

    if (0 != from && value.IsNull()) {
        jsonDescr.ConstructInPlace();
        (*jsonDescr)["type"].SetString(ERRTYPE_NULL);
        (*jsonDescr)["path"].SetString(path);
    }

    if (!jsonDescr.Defined()) {
        const ui32 v = GetNumberOfUTF8Chars(value);
        if (v < from || v > to) {
            jsonDescr.ConstructInPlace();
            (*jsonDescr)["type"].SetString(ERRTYPE_SIZE);
            (*jsonDescr)["path"].SetString(path);
            NSc::TValue& constraints = (*jsonDescr)["constraints"];
            constraints.Push(from);
            constraints.Push(to);
        }
    }

    if (jsonDescr.Defined()) {
        Data()["problems"].Push().Swap(jsonDescr.GetRef());
        return false;
    }

    return true;
}

bool TSkillValidateHelper::CheckForSize(TStringBuf path, NDomSchemeRuntime::TConstPrimitive<TSchemeTraits, typename TSchemeTraits::TStringType> value, ssize_t from, ssize_t to) {
    NSc::TValue jsonDescr;

    if (0 != from) {
        if (value.IsNull()) {
            jsonDescr["type"].SetString(ERRTYPE_NULL);
            jsonDescr["path"].SetString(path);
        }
    }

    if (jsonDescr.IsNull()) {
        const ssize_t v = value->size();
        if (v < from || v > to) {
            jsonDescr["type"].SetString(ERRTYPE_SIZE);
            jsonDescr["path"].SetString(path);
            NSc::TValue& constraints = jsonDescr["constraints"];
            constraints.Push(from);
            constraints.Push(to);
        }
    }

    if (!jsonDescr.IsNull()) {
        Data()["problems"].Push(std::move(jsonDescr));
        return false;
    }

    return true;
}

void TSkillValidateHelper::AddProblem(TStringBuf type, TStringBuf path, TStringBuf msg) {
    NSc::TValue& jsonDescr = Data()["problems"].Push();

    jsonDescr["type"].SetString(type);
    jsonDescr["path"].SetString(path);
    jsonDescr["msg"].SetString(msg);
}

bool TSkillValidateHelper::CheckForImageId(TStringBuf imageId, TStringBuf subPath) {
    if (imageId.empty()) {
        AddProblem(ERRTYPE_IMAGE_ID, subPath, "empty");
        return false;
    }

    // 64 is magic! just to prevent sending too long imageId
    if (imageId.size() > 64) {
        AddProblem(ERRTYPE_IMAGE_ID, subPath, "too long");
        return false;
    }

    str_spn rul("a-z0-9/_", true);
    if (rul.spn(imageId.data()) != imageId.size()) {
        AddProblem(ERRTYPE_IMAGE_ID, subPath, "must contain only a-z0-9/_");
        return false;
    }

    return true;
}

} // namespace NExternalSkill
} // namespace NBASS

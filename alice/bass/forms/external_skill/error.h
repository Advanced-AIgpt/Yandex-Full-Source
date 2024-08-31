#pragma once

#include <alice/bass/forms/context/fwd.h>

#include <alice/bass/util/error.h>

#include <library/cpp/scheme/scheme.h>

#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>

namespace NBASS {
namespace NExternalSkill {

/** Error types */
inline constexpr TStringBuf ERRTYPE_APIERROR = "api";
inline constexpr TStringBuf ERRTYPE_BADJSON = "bad_json";
inline constexpr TStringBuf ERRTYPE_BADURL = "bad_url";
inline constexpr TStringBuf ERRTYPE_CARD_ITEMS = "invalid_card_items";
inline constexpr TStringBuf ERRTYPE_CARD_TYPE = "invalid_card_type";
inline constexpr TStringBuf ERRTYPE_FETCH = "external_skill_http_error";
inline constexpr TStringBuf ERRTYPE_HTTP_PARSE = "external_skill_http_parse_error";
inline constexpr TStringBuf ERRTYPE_IMAGE_ID = "invalid_image_id";
inline constexpr TStringBuf ERRTYPE_MANY = "too_many_fields";
inline constexpr TStringBuf ERRTYPE_NAME_RESOLUTION_ERROR = "name_resolution_error";
inline constexpr TStringBuf ERRTYPE_NULL = "required_field_missing";
inline constexpr TStringBuf ERRTYPE_SIZE = "size_exceeded";
inline constexpr TStringBuf ERRTYPE_TIMEOUT = "external_skill_fetch_timeout";
inline constexpr TStringBuf ERRTYPE_UNKNOWN = "unknown";
inline constexpr TStringBuf ERRTYPE_VERSION = "unsupported_version";

/** A wrapper for TError.
 * Is used for erros which draws via blocks and allows pass additional data in specific format into this block
 */
class TErrorBlock {
public:
    using TResult = TMaybe<TErrorBlock>;
    TError Error;
    NSc::TValue Data;

    static const TErrorBlock::TResult Ok;

public:
    /** Creates error with empty data. It is possible to add data via <AddProblem()> method.
     */
    TErrorBlock(TError::EType type, TStringBuf errmsg);

    /** Creates SKILLSERROR error with 'problem' type and it's path.
     * The same as TErrorBlock(TError::EType::SKILLSERROR, errmsg).AddProblem(type, path);
     */
    TErrorBlock(TStringBuf errmsg, TStringBuf type, TStringBuf path);

    /** Create from already created error
     */
    explicit TErrorBlock(TError error);

    /** If developer mode is enabled it creates a pretty json and put it into
     * answer slot as text.
     */
    void InsertIntoContex(TContext* ctx) const;

    TErrorBlock& AddProblem(TStringBuf type, TStringBuf path);
    TErrorBlock& AddProblem(TStringBuf type, TStringBuf path, NSc::TValue data);

    /** Enable developer mode which affects InsertIntoContext() output.
     * @see InsertIntoContex()
     */
    void SetDeveloperMode() {
        IsDeveloperMode = true;
    }

private:
    bool IsDeveloperMode = false;
};

} // namespace NExternalSkill
} // namespace NBASS

#pragma once

#include <alice/bass/forms/context/fwd.h>

#include <alice/bass/util/error.h>

#include <library/cpp/timezone_conversion/convert.h>

namespace NBASS {
namespace NReminders {

class TRequest;

/** Struct describes common actions for different forms.
 */
struct TFormsDescr {
    using TErrorPostworkCallback = std::function<void(TContext&)>;

    const TError::EType ErrorType;
    /** Type of answer slot */
    const TStringBuf SlotType;
    /** Prefix for authorization and maybe some other things... */
    const TStringBuf Prefix;
    /** Default type in json in answer slot. It is needed for vins */
    const TStringBuf DefaultType;
    /** Default error code to to set errors, very useful ;) */
    const TStringBuf DefaultErrorCode;
    /** Additional actions when error is set. I.e. added some suggests. */
    TMaybe<TErrorPostworkCallback> PostworkErrorCallback = Nothing();
    /** This flag allows to bypass to check a support condition */
    const bool DoNotCheckForSupported = false;
    /** whether uid need or not. */
    const bool IsUidRequired = true;

    /** Add error to context with error @ErrorType and given errCode.
     * It also runs @PostworkErrorCallback if it is specified.
     * @param[out] ctx is a context where error will be written
     * @param[in] errCode is an error code
     */
    void SetError(TContext& ctx, TStringBuf errCode) const;
    /** @see SetError
     * Same behaviour as @SetError() but it don't run @PostworkErrorCallback
     */
    void SetErrorWithoutPostwork(TContext& ctx, TStringBuf errCode) const;
    /** Add error to context with error @ErrorType and @DefaultErrorCode.
     * Just very useful ;)
     */
    void SetDefaultError(TContext& ctx) const;
    /** Just create answer slot with default values and returns the ref to its json
     */
    NSc::TValue& CreateSlotInContext(TContext& ctx) const;
};

class TDateSlot {
public:
    /** Check if slot date is existed and parse it and return valid 'maybe' if everything
     * is success.
     */
    static TMaybe<TDateSlot> Create(const TRequest& request);

    /** Check is date in this slot in past. It doesn't compare times, only dates!!!
     */
    bool InPast(const NDatetime::TCivilDay& curDay) const;
    /** Clear slot value. Usually it is done after we check if the day in the past.
     * @see InPast()
     */
    void Clear();

    /** The same as strftime() but for date in slot.
     * Beware: do not try to output time value its undefined or zero, I don't know ;)
     */
    TString Format(TStringBuf format) const;

private:
    TDateSlot(NDatetime::TTimeZone tz, const NDatetime::TCivilSecond& day, TSlot* const slot)
        : TZ(tz)
        , Day(day)
        , Slot(slot)
    {
    }

    NDatetime::TTimeZone TZ;
    NDatetime::TCivilDay Day;
    TSlot* Slot;
};

class TAnswerSlot {
public:
    explicit TAnswerSlot(TRequest& req);
    ~TAnswerSlot();

    NSc::TValue& Json();
    TAnswerSlot& SetDefaultType();

    static constexpr TStringBuf SlotName() {
        return TStringBuf("answer");
    }

private:
    TContext& Ctx;
    const TFormsDescr& DescrRef;
    NSc::TValue* JsonRef;
};

class TRequest {
public:
    struct THandler {
        const std::function<bool(TContext&)> CheckIfSuitable;
        const std::function<void(TRequest&, TContext&)> Handle;
        const TFormsDescr Descr;
    };

public:
    /** It creates request by checking if user is authorized and requests its UID from blackbox.
     * In case of any error it adds error into conetext and return undef maybe.
     * @param[in|out] ctx is the main context
     * @param[in] descr is a descripton of current form/action. request doesn't not own but hold a ref to it, so beware!
     * @return constructed request or undefinded maybe in case of any error
     */
    static TMaybe<TRequest> Create(TContext& ctx, const TFormsDescr& descr);
    static const THandler* Handle(TContext& ctx, const TVector<THandler>& handlers);

    TAnswerSlot& AnswerSlot();
    TDateSlot* DateSlot();

    NDatetime::TCivilSecond CurrentTime() const;
    TInstant CurrentTimestamp() const {
        return CurrentEpoch;
    }

    void SetDefaultError() {
        DescrRef.SetDefaultError(Ctx);
    }

    void SetError(TStringBuf errCode) {
        DescrRef.SetError(Ctx, errCode);
    }

    void SetErrorWithoutPostwork(TStringBuf errCode) {
        DescrRef.SetErrorWithoutPostwork(Ctx, errCode);
    }

    TContext* operator->() {
        return &Ctx;
    }

    TContext* operator*() {
        return &Ctx;
    }

    const TFormsDescr& Descr() const {
        return DescrRef;
    }

    TContext& Ctx;
    const TString Uid;
    const NDatetime::TTimeZone UserTZ;

    /** Todos and reminders have two behaviour models (search app and quasar).
     * Right now it supports only one function and else in if ;)
     */
    static bool IsSearchAppBehavior(const TClientInfo& ctx);

    static void ListStopAction(TContext& ctx, const TFormsDescr& descr);

    // Helpers.
    static void ClearSlot(TSlot* slot);
    void MoveSlotValueToAnswer(const TStringBuf slotName, const TStringBuf slotType);
    void ClearSlot(const TStringBuf slotName, const TStringBuf slotType);
    void CopySlotValueFromAnswer(const TStringBuf slotName, const TStringBuf slotType);

    TMaybe<TString> ObtainRemindText();
    TMaybe<TStringBuf> ProcessEmptySlotWhatValue();

private:
    TRequest(TContext& ctx, TString uid, NDatetime::TTimeZone tz, const TFormsDescr& descr);

private:
    const TFormsDescr& DescrRef;
    const NDatetime::TCivilSecond CurrentDateTime;
    const TInstant CurrentEpoch;
    TMaybe<TAnswerSlot> AnswerSlotHolder;
    TMaybe<TDateSlot> DateSlotHolder;
};

} // namespace NReminders
} // namespace NBASS

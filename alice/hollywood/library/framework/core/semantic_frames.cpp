//
// HOLLYWOOD FRAMEWORK
// Advanced semantic frames parser
//
#include "semantic_frames.h"

#include "request.h"

#include <alice/hollywood/library/frame/callback.h>

#include <alice/library/json/json.h>
#include <alice/library/sys_datetime/sys_datetime.h>
#include <alice/megamind/protos/scenarios/request.pb.h>

#include <google/protobuf/struct.pb.h>

#include <util/string/cast.h>

namespace NAlice::NHollywoodFw {

namespace {

template <class T>
T StandardSlotHelper(const NPrivate::TBasedSlot* slot, T defaultValue) {
    if (slot->Defined()) {
        T value;
        return TryFromString(slot->GetValue(), value) ? value : defaultValue;
    }
    return defaultValue;
}

template <class T>
TMaybe<T> OptionalSlotHelper(const NPrivate::TBasedSlot* slot) {
    if (!slot->Defined()) {
        return Nothing();
    }
    T value;
    return TryFromString(slot->GetValue(), value) ? TMaybe<T>(value) : Nothing();
}

template <class T>
TVector<T> ArraySlotHelper(const TFrame* owner, TStringBuf slotName) {
    TVector<T> result;
    const NAlice::TSemanticFrame* frame = owner->GetFrame();
    if (frame == nullptr) {
        return result;
    }
    T value;
    for (const auto& it : frame->GetSlots()) {
        if (it.GetName() == slotName) {
            if (TryFromString(it.GetValue(), value)) {
                result.push_back(value);
            }
        }
    }
    return result;
}

} // anonimous namespace

/*
    TFrame ctor
    Creates semantic frame from user input
    For more complex versions see TFrame::TFrameWithCallback() below
*/
TFrame::TFrame(const TRequest::TInput& input, TStringBuf frameName)
    : Frame_(nullptr)
{
    for (const auto& it : input.GetInputProto().GetSemanticFrames()) {
        if (it.GetName() == frameName) {
            Frame_ = &it;
            break;
        }
    }
}

/*
    Create a frame using either callback frame or standard frame
    This constructor first try to create a frame from callback frame
        * callback must have FRAME_CALLBACK name
        * callback payload must contains json frame with `frameName`
    If callback is not found, will try to create an usual semantic frame from Input
*/
TFrame::TFrame(const TRequest& request, TStringBuf frameName, EFrameConstructorMode mode)
    : Frame_(nullptr)
{
    switch (mode) {
        case EFrameConstructorMode::Empty:
            FrameHolder_.reset(new NAlice::TSemanticFrame());
            FrameHolder_->SetName(TString{frameName});
            Frame_ = FrameHolder_.get();
            return;
        case EFrameConstructorMode::FrameWithCallback: {
            const NHollywood::TPtrWrapper<NScenarios::TCallbackDirective> cb = request.Input().FindCallback();
            if (cb && cb->GetName() == NHollywood::FRAME_CALLBACK) {
                const auto str = cb->GetPayload().fields().at("frame").string_value();
                const auto& semanticFrame = JsonToProto<TSemanticFrame>(JsonFromString(str));
                if (semanticFrame.GetName() == frameName) {
                    FrameHolder_.reset(new NAlice::TSemanticFrame(std::move(semanticFrame)));
                    Frame_ = FrameHolder_.get();
                    return;
                }
            }
            break;
        }
    }
    // Callback not found, create standard frame
    for (const auto& it : request.Input().GetInputProto().GetSemanticFrames()) {
        if (it.GetName() == frameName) {
            Frame_ = &it;
            break;
        }
    }
}

/*
    Additional TFrame ctor
    Can be used for unit testing purposes
*/
TFrame::TFrame(const NAlice::TSemanticFrame* proto)
    : Frame_(proto)
{
}

TFrame::~TFrame() {
}

const TString TFrame::GetName() const {
    if (Frame_ == nullptr) {
        return TString{};
    }
    return Frame_->GetName();
}

const NAlice::TSemanticFrame::TSlot* TFrame::FindSlot(TStringBuf slotName) const {
    if (!Defined()) {
        return nullptr;
    }
    for (const auto& it : Frame_->GetSlots()) {
        if (it.GetName() == slotName) {
            return &it;
        }
    }
    return nullptr;
}

/*
    TBasedSlot ctor
    Internal function
    Stores Name, Type, Value and
*/
NPrivate::TBasedSlot::TBasedSlot(const TFrame* owner, TStringBuf slotName)
    : Defined_(false)
    , Name_(slotName)
{
    const auto* slot = owner->FindSlot(slotName);
    if (slot != nullptr) {
        Defined_ = true;
        Type_ = slot->GetType();
        Value_ = slot->GetValue();
    }
}

//
// TSlot
//
template <>
TSlot<bool>::TSlot(const TFrame* owner, TStringBuf slotName, bool defaultValue /*= 0*/)
    : NPrivate::TBasedSlot(owner, slotName)
{
    Value = StandardSlotHelper<i32>(this, defaultValue);
}
template <>
TSlot<i32>::TSlot(const TFrame* owner, TStringBuf slotName, i32 defaultValue /*= 0*/)
    : NPrivate::TBasedSlot(owner, slotName)
{
    Value = StandardSlotHelper<i32>(this, defaultValue);
}
template <>
TSlot<ui32>::TSlot(const TFrame* owner, TStringBuf slotName, ui32 defaultValue /*= 0*/)
    : NPrivate::TBasedSlot(owner, slotName)
{
    Value = StandardSlotHelper<ui32>(this, defaultValue);
}
template <>
TSlot<i64>::TSlot(const TFrame* owner, TStringBuf slotName, i64 defaultValue /*= 0*/)
    : NPrivate::TBasedSlot(owner, slotName)
{
    Value = StandardSlotHelper<i64>(this, defaultValue);
}
template <>
TSlot<ui64>::TSlot(const TFrame* owner, TStringBuf slotName, ui64 defaultValue /*= 0*/)
    : NPrivate::TBasedSlot(owner, slotName)
{
    Value = StandardSlotHelper<ui64>(this, defaultValue);
}
template <>
TSlot<float>::TSlot(const TFrame* owner, TStringBuf slotName, float defaultValue /*= 0*/)
    : NPrivate::TBasedSlot(owner, slotName)
{
    Value = StandardSlotHelper<float>(this, defaultValue);
}
template <>
TSlot<TString>::TSlot(const TFrame* owner, TStringBuf slotName, TString defaultValue /*= ""*/)
    : NPrivate::TBasedSlot(owner, slotName)
{
    Value = StandardSlotHelper<TString>(this, defaultValue);
}

//
// TOptionalSlot
//
template <>
TOptionalSlot<bool>::TOptionalSlot(const TFrame* owner, TStringBuf slotName)
    : NPrivate::TBasedSlot(owner, slotName)
{
    Value = OptionalSlotHelper<bool>(this);
}
template <>
TOptionalSlot<i32>::TOptionalSlot(const TFrame* owner, TStringBuf slotName)
    : NPrivate::TBasedSlot(owner, slotName)
{
    Value = OptionalSlotHelper<i32>(this);
}
template <>
TOptionalSlot<ui32>::TOptionalSlot(const TFrame* owner, TStringBuf slotName)
    : NPrivate::TBasedSlot(owner, slotName)
{
    Value = OptionalSlotHelper<ui32>(this);
}
template <>
TOptionalSlot<i64>::TOptionalSlot(const TFrame* owner, TStringBuf slotName)
    : NPrivate::TBasedSlot(owner, slotName)
{
    Value = OptionalSlotHelper<i64>(this);
}
template <>
TOptionalSlot<ui64>::TOptionalSlot(const TFrame* owner, TStringBuf slotName)
    : NPrivate::TBasedSlot(owner, slotName)
{
    Value = OptionalSlotHelper<ui64>(this);
}
template <>
TOptionalSlot<float>::TOptionalSlot(const TFrame* owner, TStringBuf slotName)
    : NPrivate::TBasedSlot(owner, slotName)
{
    Value = OptionalSlotHelper<float>(this);
}
template <>
TOptionalSlot<TString>::TOptionalSlot(const TFrame* owner, TStringBuf slotName)
    : NPrivate::TBasedSlot(owner, slotName)
{
    Value = OptionalSlotHelper<TString>(this);
}
template <>
TOptionalSlot<TSysDatetimeParser>::TOptionalSlot(const TFrame* owner, TStringBuf slotName)
    : NPrivate::TBasedSlot(owner, slotName)
{
    Value = TSysDatetimeParser::Parse(GetValue());
}

//
// TArraySlot
//
template <>
TArraySlot<bool>::TArraySlot(const TFrame* owner, TStringBuf slotName)
    : NPrivate::TBasedSlot(owner, slotName)
    , Value(ArraySlotHelper<bool>(owner, slotName))
{
}
template <>
TArraySlot<i32>::TArraySlot(const TFrame* owner, TStringBuf slotName)
    : NPrivate::TBasedSlot(owner, slotName)
    , Value(ArraySlotHelper<i32>(owner, slotName))
{
}
template <>
TArraySlot<ui32>::TArraySlot(const TFrame* owner, TStringBuf slotName)
    : NPrivate::TBasedSlot(owner, slotName)
    , Value(ArraySlotHelper<ui32>(owner, slotName))
{
}
template <>
TArraySlot<i64>::TArraySlot(const TFrame* owner, TStringBuf slotName)
    : NPrivate::TBasedSlot(owner, slotName)
    , Value(ArraySlotHelper<i64>(owner, slotName))
{
}
template <>
TArraySlot<ui64>::TArraySlot(const TFrame* owner, TStringBuf slotName)
    : NPrivate::TBasedSlot(owner, slotName)
    , Value(ArraySlotHelper<ui64>(owner, slotName))
{
}
template <>
TArraySlot<float>::TArraySlot(const TFrame* owner, TStringBuf slotName)
    : NPrivate::TBasedSlot(owner, slotName)
    , Value(ArraySlotHelper<float>(owner, slotName))
{
}
template <>
TArraySlot<TString>::TArraySlot(const TFrame* owner, TStringBuf slotName)
    : NPrivate::TBasedSlot(owner, slotName)
    , Value(ArraySlotHelper<TString>(owner, slotName))
{
}
template <>
TArraySlot<TSysDatetimeParser>::TArraySlot(const TFrame* owner, TStringBuf slotName)
    : NPrivate::TBasedSlot(owner, slotName)
{
    const NAlice::TSemanticFrame* frame = owner->GetFrame();
    if (frame != nullptr) {
        for (const auto& it : frame->GetSlots()) {
            if (it.GetName() == slotName) {
                TMaybe<TSysDatetimeParser> value = TSysDatetimeParser::Parse(it.GetValue());
                if (value.Defined()) {
                    Value.push_back(*value);
                }
            }
        }
    }
}

} // namespace NAlice::NHollywoodFw

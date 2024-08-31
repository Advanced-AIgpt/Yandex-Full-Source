#pragma once

//
// HOLLYWOOD FRAMEWORK
// Advances semantic frames parser
//

#include "error.h"
#include "request.h"

#include <util/generic/map.h>
#include <util/generic/maybe.h>
#include <util/generic/set.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>

//
// Forward declarations
//
namespace NAlice {

class TSemanticFrame;
class TSemanticFrame_TSlot;

} // namespace NAlice

namespace NAlice::NHollywoodFw {

//
// Forward declarations
//
class TRequest;
class TFrame;

namespace NPrivate {

/*
    TFrame provides following standard template slots:
    - TSlot - filled slot with optional default value
    - TOptionalSlot - contains Maybe<> value
    - TArraySlot - contains repeated values

    Also TFrame supports other standard slots like sys.date, etc
*/
class TBasedSlot {
public:
    TBasedSlot(const TFrame* owner, TStringBuf slotName);

    bool Defined() const {
        return Defined_;
    }
    const TString& GetName() const {
        return Name_;
    }
    const TString& GetType() const {
        return Type_;
    }
    const TString& GetValue() const {
        return Value_;
    }
protected:
    bool Defined_ = false;
private:
    TString Name_;
    TString Type_;
    TString Value_;
};

} // namespace NPrivate

/*
    Additional classes for experimental Frame/Slot wrapper
*/
class TFrame {
public:
    //
    // Various types to construct semantic frame
    //
    enum class EFrameConstructorMode {
        // Don't check and parse slots, this frame will be Defined(), but doesn't contains any valid values
        Empty,
        // First try to analyze frame in callback, next check usual semantic frame
        FrameWithCallback
    };
    // Standard ctor for scenarios
    TFrame(const TRequest::TInput& input, TStringBuf frameName);
    // Ctor for semantic frames with special sources
    TFrame(const TRequest& request, TStringBuf frameName, EFrameConstructorMode mode);
    // Additional ctor for testing purposes
    TFrame(const NAlice::TSemanticFrame* proto);
    ~TFrame();

    bool Defined() const {
        return Frame_ != nullptr;
    }
    const NAlice::TSemanticFrame* GetFrame() const {
        return Frame_;
    }
    const TString GetName() const;

    // Internal function for TSlotXxx
    const NAlice::TSemanticFrame::TSlot* FindSlot(TStringBuf slotName) const;

private:
    const NAlice::TSemanticFrame* Frame_;
    // Additional holder if semantic frame creates on fly
    std::unique_ptr<NAlice::TSemanticFrame> FrameHolder_;
};

template <class T>
struct TSlot : public NPrivate::TBasedSlot {
    explicit TSlot(const TFrame* owner, TStringBuf slotName, T defaultValue);
    T Value;
};

template <class T>
struct TOptionalSlot : public NPrivate::TBasedSlot {
    explicit TOptionalSlot(const TFrame* owner, TStringBuf slotName);
    bool Defined() const {
        return Value.Defined();
    }
    const T& operator *() const {
        return *Value;
    }
    TMaybe<T> Value;

    constexpr explicit operator bool() const noexcept {
        return Defined();
    }
};

template <class T>
struct TArraySlot : public NPrivate::TBasedSlot {
    explicit TArraySlot(const TFrame* owner, TStringBuf slotName);
    TVector<T> Value;
};

template <typename E>
struct TEnumSlot : public NPrivate::TBasedSlot {
    explicit TEnumSlot(const TFrame* owner, TStringBuf slotName, const TMap<TStringBuf, E>& mapper)
        : NPrivate::TBasedSlot(owner, slotName)
    {
        Y_ENSURE(!mapper.empty(), "Slotvalue mapper can not be empty object!");
        auto it = mapper.find(GetValue());
        if (it == mapper.end()) {
            // Use empty value of mapper or throw an exception
            it = mapper.find("");
            if (it == mapper.end()) {
                HW_ERROR("Undefined mapping for enum slot '" << TString(slotName) << "', SF: '" << owner->GetName() << "', value: " << GetValue());
            }
        }
        Value = it->second;
    }
    E Value;
};

} // namespace NAlice::NHollywoodFw

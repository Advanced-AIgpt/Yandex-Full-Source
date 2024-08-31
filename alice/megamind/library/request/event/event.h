#pragma once

#include <alice/megamind/protos/common/events.pb.h>
#include <alice/megamind/protos/scenarios/request.pb.h>

#include <util/generic/maybe.h>
#include <util/generic/ptr.h>
#include <util/generic/singleton.h>
#include <util/generic/string.h>
#include <util/generic/yexception.h>

#include <memory>

namespace NAlice {

class TServerActionEvent;

class IEvent {
public:
    struct TInvalidEvent : public yexception {
        using yexception::yexception;
    };

public:
    explicit IEvent(const TEvent& event)
        : Event_(event)
    {
    }

    virtual ~IEvent() = default;

    virtual const TString& GetUtterance() const {
        return Default<TString>();
    }

    virtual TStringBuf GetAsrNormalizedUtterance() const {
        return {};
    }

    virtual bool HasUtterance() const {
        return false;
    }

    virtual bool HasAsrNormalizedUtterance() const {
        return false;
    }

    virtual bool IsUserGenerated() const {
        return true;
    }

    virtual bool IsTextInput() const {
        return false;
    }

    virtual bool IsVoiceInput() const {
        return false;
    }

    virtual bool IsImageInput() const {
        return false;
    }

    virtual bool IsMusicInput() const {
        return false;
    }

    virtual bool IsAsrWhisper() const {
        return false;
    }

    virtual const TServerActionEvent* AsServerActionEvent() const {
        return nullptr;
    }

    // FIXME(the0): It's better to construct new event based on normalized utterance when it is available.
    virtual void FillScenarioInput(const TMaybe<TString>& normalizedUtterance, NScenarios::TInput* input) const = 0;

    const TEvent& SpeechKitEvent() const {
        return Event_;
    }

    static std::unique_ptr<IEvent> CreateEvent(const TEvent& event);

private:
    TEvent Event_;
};

} // namespace NAlice

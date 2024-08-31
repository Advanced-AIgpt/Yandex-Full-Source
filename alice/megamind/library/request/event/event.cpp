#include "event.h"

#include "image_input_event.h"
#include "music_input_event.h"
#include "server_action_event.h"
#include "suggested_input_event.h"
#include "text_input_event.h"
#include "voice_input_event.h"

namespace NAlice {

std::unique_ptr<IEvent> IEvent::CreateEvent(const TEvent& event) {
    if (!event.IsInitialized() || !event.HasType()) {
        return {};
    }

    switch (event.GetType()) {
        case EEventType::text_input:
            return std::make_unique<TTextInputEvent>(event);
        case EEventType::voice_input:
            return std::make_unique<TVoiceInputEvent>(event);
        case EEventType::suggested_input:
            return std::make_unique<TSuggestedInputEvent>(event);
        case EEventType::music_input:
            return std::make_unique<TMusicInputEvent>(event);
        case EEventType::image_input:
            return std::make_unique<TImageInputEvent>(event);
        case EEventType::server_action:
            return std::make_unique<TServerActionEvent>(event);
    }
}

} // namespace NAlice

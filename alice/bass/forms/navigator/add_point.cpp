#include "add_point.h"
#include "navigator_intent.h"

#include <alice/bass/forms/directives.h>
#include <alice/bass/forms/geo_resolver.h>

#include <alice/library/analytics/common/product_scenarios.h>

namespace NBASS {

static constexpr TStringBuf ADD_POINT = "personal_assistant.navi.add_point";
static constexpr TStringBuf ADD_POINT_ELLIPSIS = "personal_assistant.navi.add_point__ellipsis";

class TAddPointIntent : public INavigatorIntent {
public:
    enum class ECategory {
        Accident = 0,
        RoadWorks = 1,
        Camera = 2,
        Other = 3,
        Blocked = 4,
        RisedBridge = 5,
        Comment = 6,
        Feedback = 7
    };

    TAddPointIntent(TContext& ctx, ECategory category, TMaybe<TGeoPosition> pos, TStringBuf comment)
        : INavigatorIntent(ctx, TStringBuf("add_point") /* scheme */)
        , Category(category)
        , Pos(pos)
        , Comment(comment)
    {}

private:
    TResultValue SetupSchemeAndParams() override {
        Params.InsertUnescaped(TStringBuf("category"), ToString(int(Category)));
        if (Pos) {
            Params.InsertUnescaped(TStringBuf("lat"), ToString(Pos->Lat));
            Params.InsertUnescaped(TStringBuf("lon"), ToString(Pos->Lon));
        }
        if (!Comment.empty()) {
            Params.InsertUnescaped(TStringBuf("comment"), Comment);
        }

        Params.InsertUnescaped(TStringBuf("force_publish"), TStringBuf("1"));

        return TResultValue();
    }

    TDirectiveFactory::TDirectiveIndex GetDirectiveIndex() override {
        return GetAnalyticsTagIndex<TNavigatorAddPointDirective>();
    }
private:
    ECategory Category;
    TMaybe<TGeoPosition> Pos;
    TString Comment;
};

TResultValue TNavigatorAddPointHandler::Do(TRequestHandler& r) {
    static const THashMap<TString, TAddPointIntent::ECategory> eventTypes = {
        {"traffic_accidents", TAddPointIntent::ECategory::Accident},
        {"road_works", TAddPointIntent::ECategory::RoadWorks},
        {"camera", TAddPointIntent::ECategory::Camera},
        {"talk", TAddPointIntent::ECategory::Comment},
        {"error", TAddPointIntent::ECategory::Feedback},
        {"error_no_route", TAddPointIntent::ECategory::Feedback},
        {"error_no_turn", TAddPointIntent::ECategory::Feedback},
        {"other", TAddPointIntent::ECategory::Other}
    };

    r.Ctx().GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::NAVI_COMMANDS);

    TContext::TSlot* eventSlot = r.Ctx().GetOrCreateSlot("road_event", "road_event");
    if (IsSlotEmpty(eventSlot)) {
        eventSlot->Optional = false;
        return TResultValue();
    }

    TStringBuf eventStr = eventSlot->Value.GetString();
    THashMap<TString, TAddPointIntent::ECategory>::const_iterator event = eventTypes.find(eventStr);

    if (event == eventTypes.cend()) {
        return TError(TError::EType::SYSTEM,
                      TStringBuilder() << "Unknown road event type : " << eventStr);
    }

    // remember initial location
    TMaybe<TGeoPosition> pos;
    TContext::TSlot* positionSlot = r.Ctx().GetOrCreateSlot("location", "location");
    if (IsSlotEmpty(positionSlot)) {
        pos = InitGeoPositionFromLocation(r.Ctx().Meta().Location());
        if (pos) {
            positionSlot->Value = pos->ToJson();
        }
    } else {
        pos = TGeoPosition::FromJson(positionSlot->Value);
    }

    TContext::TSlot* commentSlot = r.Ctx().GetOrCreateSlot("comment", "string");
    if (IsSlotEmpty(commentSlot)) {
        commentSlot->Optional = false;
        return TResultValue();
    }

    TAddPointIntent::ECategory eventId = event->second;
    switch (eventId) {
    case TAddPointIntent::ECategory::Accident :
    case TAddPointIntent::ECategory::RoadWorks : {
        TContext::TSlot* lane = r.Ctx().GetOrCreateSlot("lane", "string");
        if (IsSlotEmpty(lane)) {
            lane->Optional = false;
            return TResultValue();
        }
        break;
    }
    default:
        break;
    }

    TAddPointIntent navigatorIntent(r.Ctx(), eventId, pos, commentSlot->Value.GetString());
    return navigatorIntent.Do();
}

void TNavigatorAddPointHandler::Register(THandlersMap* handlers) {
    auto cbNavigatorAddPointForm = []() {
        return MakeHolder<TNavigatorAddPointHandler>();
    };

    handlers->emplace(ADD_POINT, cbNavigatorAddPointForm);
    handlers->emplace(ADD_POINT_ELLIPSIS, cbNavigatorAddPointForm);
}

}

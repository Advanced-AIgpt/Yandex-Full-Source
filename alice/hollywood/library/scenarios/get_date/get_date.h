#pragma once

#include <alice/hollywood/library/scenarios/get_date/proto/get_date.pb.h>
#include <alice/hollywood/library/scenarios/get_date/slot_utils/slot_utils.h>

#include <alice/hollywood/library/framework/framework.h>

#include <alice/library/sys_datetime/sys_datetime.h>

namespace NAlice::NHollywoodFw::NGetDate {

inline constexpr TStringBuf SCENE_NAME_DEFAULT = "default";

inline constexpr TStringBuf FRAME_GET_DATE = "personal_assistant.scenarios.get_date";
inline const TString FRAME_GET_DATE_ELLIPSIS = "personal_assistant.scenarios.get_date__ellipsis";


class TGetDateScene : public TScene <TGetDateSceneArgs> {
public:
    TGetDateScene(const TScenario* owner);
    TRetMain Main(const TGetDateSceneArgs&,
                  const TRunRequest&,
                  TStorage&,
                  const TSource&) const override;
private:
    TRetMain Finalize(TRTLogger& logger,
                      ETargetQuestion question,
                      TSysDatetimeParser::ETense tense,
                      TSysDatetimeParser& dateResult,
                      const NDatetime::TCivilSecond dateCurrent,
                      TGetDateRenderProto& renderProto) const;
};

class TGetDateScenario : public TScenario {
public:
    TGetDateScenario();

    TRetScene Dispatch(const TRunRequest&,
                       const TStorage&,
                       const TSource&) const;
    static TRetResponse Render(const TGetDateRenderProto&, TRender&);
};

}  // namespace NAlice::NHollywood::NGetDate

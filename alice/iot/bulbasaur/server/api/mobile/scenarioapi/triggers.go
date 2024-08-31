package scenarioapi

import (
	"io/ioutil"
	"net/http"
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/scenario"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/mobile"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/render"
	"a.yandex-team.ru/alice/iot/bulbasaur/server/apierrors"
	"a.yandex-team.ru/alice/library/go/binder"
	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/valid"
)

type Handlers struct {
	logger             log.Logger
	renderer           render.Renderer
	scenarioController scenario.IController
}

func NewHandlers(logger log.Logger, renderer render.Renderer, scenarioController scenario.IController) *Handlers {
	return &Handlers{
		logger:             logger,
		renderer:           renderer,
		scenarioController: scenarioController,
	}
}

// swagger:operation Post /m/user/scenarios/triggers/calculate/solar Scenarios CalculateSolarTriggerValue
// calculate next run for timetable trigger
//
// ---
// produces:
// - application/json
// parameters:
// - name: payload
//   in: body
//   description: json payload
//   schema:
//     $ref: "#/definitions/SolarCalculationRequest"
// responses:
//   "200":
//     description: success calculation
//     schema:
//       "$ref": "#/definitions/SolarCalculationResponse"
//     examples:
//       application/json:
//         status: ok
//         request_id: 8e9646cc-cfee-4c22-8f0d-01064b8147b5
//         time: 2022-06-12T14:04:00Z
func (h *Handlers) CalculateSolarTriggerValue(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	body, err := ioutil.ReadAll(r.Body)
	if err != nil {
		ctxlog.Warnf(ctx, h.logger, "error reading body: %v", err)
		h.renderer.RenderMobileError(ctx, w, &apierrors.ErrBadRequest{})
		return
	}

	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warn(ctx, h.logger, err.Error())
		h.renderer.RenderMobileError(ctx, w, &apierrors.ErrUnauthorized{})
		return
	}

	vctx := valid.NewValidationCtx()
	var data mobile.SolarCalculationRequest
	if err = binder.Bind(vctx, body, &data); err != nil {
		ctxlog.Warnf(ctx, h.logger, "wrong request format: %v", err)
		if xerrors.Is(err, model.ValidationError) {
			h.renderer.RenderMobileError(ctx, w, err)
			return
		}
		h.renderer.RenderMobileError(ctx, w, &apierrors.ErrBadRequest{})
		return
	}

	triggerValue := model.TimetableTriggerValue{
		Condition: model.TimetableTriggerConditionWrapper{
			Type: model.SolarTimeConditionType,
			Value: model.SolarConditionValue{
				Weekdays:      data.Weekdays,
				Solar:         data.Solar,
				HouseholdID:   data.HouseholdID,
				OffsetSeconds: 0, // offset is always zero for calculation
			},
		},
	}

	trigger, err := triggerValue.ToTrigger()
	if err != nil {
		ctxlog.Warnf(ctx, h.logger, "failed to load trigger value: %v", err)
		h.renderer.RenderMobileError(ctx, w, &apierrors.ErrBadRequest{})
		return
	}

	nextRun, err := h.scenarioController.CalculateNextTimetableRun(ctx, user, []model.TimetableScenarioTrigger{trigger})
	if err != nil {
		ctxlog.Warnf(ctx, h.logger, "failed to calculate trigger value: %v", err)
		h.renderer.RenderMobileError(ctx, w, err)
		return
	}

	response := mobile.SolarCalculationResponse{
		Status:    "ok",
		RequestID: requestid.GetRequestID(ctx),
		Time:      nextRun.TimeUTC.Format(time.RFC3339),
	}
	h.renderer.RenderJSON(ctx, w, response)
}

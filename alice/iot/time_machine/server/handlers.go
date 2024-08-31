package timemachine

import (
	"encoding/json"
	"io/ioutil"
	"net/http"
	"strconv"

	"a.yandex-team.ru/alice/iot/time_machine/dto"
	"a.yandex-team.ru/alice/iot/time_machine/tasks"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

func (t *TimeMachine) submitTaskHandler(w http.ResponseWriter, r *http.Request) {
	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(r.Context(), t.Logger, "Error reading body: %v", err)
		http.Error(w, http.StatusText(http.StatusBadRequest), http.StatusBadRequest)
		return
	}

	var req dto.TaskSubmitRequest
	err = json.Unmarshal(body, &req)
	if err != nil {
		ctxlog.Warnf(r.Context(), t.Logger, "Error parsing body: %v", err)
		http.Error(w, http.StatusText(http.StatusBadRequest), http.StatusBadRequest)
		return
	}

	userID := strconv.FormatUint(req.UserID, 10)
	payload := req.ToTimeMachineHTTPCallbackPayload()

	id, err := t.queue.SubmitTask(r.Context(), userID, tasks.TimeMachineHTTPCallbackTask, req.MergeKey, payload, req.ScheduleTime)
	if err != nil {
		t.Logger.Warnf("Error while submitting task: %v", err)
		http.Error(w, http.StatusText(http.StatusInternalServerError), http.StatusInternalServerError)
		return
	}

	t.Logger.Infof("Submitted task %s with id %s (schedule time %s)", tasks.TimeMachineHTTPCallbackTask, id, req.ScheduleTime)
	w.WriteHeader(http.StatusOK)
}

package server

import (
	"io/ioutil"
	"net/http"
	"net/url"
	"strconv"
	"sync"

	"a.yandex-team.ru/library/go/core/log/ctxlog"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/takeout"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

func (s *Server) buildUserTakeout(w http.ResponseWriter, r *http.Request) {
	sendError := func(code int) {
		s.render.RenderJSONError(r.Context(), w, code, takeout.ResponseView{
			Status: "error",
			Error:  http.StatusText(code),
		})
	}

	//Parse request data
	body, err := ioutil.ReadAll(r.Body)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Error reading body: %s", err)
		sendError(http.StatusBadRequest)
		return
	}

	values, err := url.ParseQuery(string(body))
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Error while parsing body: %s", err)
		sendError(http.StatusBadRequest)
		return
	}

	userID, err := strconv.ParseUint(values.Get("uid"), 10, 64)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Error while reading uid from body: %s", err)
		sendError(http.StatusBadRequest)
		return
	}

	//Collect data
	data := takeout.DataView{}
	var (
		devices         []model.Device
		devicesError    error
		groups          []model.Group
		groupsError     error
		rooms           []model.Room
		roomsError      error
		scenarios       []model.Scenario
		scenariosError  error
		networks        []model.Network
		networksError   error
		households      []model.Household
		householdsError error
	)

	wg := sync.WaitGroup{}

	wg.Add(1)
	go func() {
		defer wg.Done()
		devices, devicesError = s.db.SelectUserDevices(r.Context(), userID)
		if devicesError == nil {
			data.PopulateDevices(devices)
		}
	}()

	wg.Add(1)
	go func() {
		defer wg.Done()
		groups, groupsError = s.db.SelectUserGroups(r.Context(), userID)
		if groupsError == nil {
			data.PopulateGroups(groups)
		}
	}()

	wg.Add(1)
	go func() {
		defer wg.Done()
		rooms, roomsError = s.db.SelectUserRooms(r.Context(), userID)
		if roomsError == nil {
			data.PopulateRooms(rooms)
		}
	}()

	wg.Add(1)
	go func() {
		defer wg.Done()
		scenarios, scenariosError = s.db.SelectUserScenarios(r.Context(), userID)
		if scenariosError == nil {
			data.PopulateScenarios(scenarios)
		}
	}()

	wg.Add(1)
	go func() {
		defer wg.Done()
		networks, networksError = s.db.SelectUserNetworks(r.Context(), userID)
		if networksError == nil {
			data.PopulateNetworks(networks)
		}
	}()

	wg.Add(1)
	go func() {
		defer wg.Done()
		households, householdsError = s.db.SelectUserHouseholds(r.Context(), userID)
		if householdsError == nil {
			data.PopulateHouseholds(households)
		}
	}()

	wg.Wait()

	if devicesError != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Error while collecting devices for takeout: %s", devicesError)
		sendError(http.StatusInternalServerError)
		return
	}
	if groupsError != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Error while collecting groups for takeout: %s", groupsError)
		sendError(http.StatusInternalServerError)
		return
	}
	if roomsError != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Error while collecting rooms for takeout: %s", roomsError)
		sendError(http.StatusInternalServerError)
		return
	}
	if scenariosError != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Error while collecting scenarios for takeout: %s", scenariosError)
		sendError(http.StatusInternalServerError)
		return
	}
	if networksError != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Error while collecting networks for takeout: %s", networksError)
		sendError(http.StatusInternalServerError)
		return
	}
	if householdsError != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Error while collecting households for takeout: %s", householdsError)
		sendError(http.StatusInternalServerError)
		return
	}

	//Build response
	response := takeout.ResponseView{}
	err = response.FromData(data)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Error while building takeout response: %s", err)
		sendError(http.StatusInternalServerError)
		return
	}

	s.render.RenderJSON(r.Context(), w, response)
}

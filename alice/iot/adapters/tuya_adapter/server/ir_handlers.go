package tuya

import (
	"context"
	"encoding/json"
	"fmt"
	"github.com/go-chi/chi/v5"
	"io/ioutil"
	"net/http"
	"runtime/debug"
	"sort"
	"sync"

	"a.yandex-team.ru/alice/iot/adapters/tuya_adapter/dto/mobile"
	"a.yandex-team.ru/alice/iot/adapters/tuya_adapter/middleware"
	"a.yandex-team.ru/alice/iot/adapters/tuya_adapter/tuya"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/provider"
	"a.yandex-team.ru/alice/iot/bulbasaur/server/apierrors"
	"a.yandex-team.ru/alice/library/go/binder"
	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/valid"
)

func (s *Server) IrCategoriesHandler(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	// simple user check
	skillID := middleware.GetSkillID(r.Context())
	tuyaUserID, err := s.db.GetTuyaUserID(r.Context(), user.ID, skillID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get Tuya user for uid %d. Reason: %s", user.ID, err.Error())
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	// Check user rights on device
	deviceID := chi.URLParam(r, "device_id") // external (tuya) IR device id

	userDevices, err := s.getUserDevices(r.Context(), tuyaUserID, skillID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get Tuya devices for uid %d. Reason: %s", user.ID, err.Error())
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	if _, ok := userDevices[deviceID]; !ok {
		ctxlog.Warnf(r.Context(), s.Logger, "User %d has no rights to control device %s", user.ID, deviceID)
		s.render.RenderMobileError(r.Context(), w, provider.ErrorDeviceNotFound)
		return
	}

	categoriesMap, err := s.tuyaClient.GetIrCategories(r.Context(), deviceID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get remote categories for IR device with id %s. Reason: %s", deviceID, err.Error())
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	categories := make([]mobile.IRCategory, 0, len(categoriesMap))
	for id := range categoriesMap {
		if irCategory, found := mobile.KnownIRCategories[id]; found {
			categories = append(categories, irCategory)
		}
	}

	sort.Sort(mobile.IRCategoriesByName(categories))
	// to assure custom is last option
	categories = append(categories, mobile.CustomIRCategory)

	response := mobile.IRCategoriesResponse{
		RequestID:  requestid.GetRequestID(r.Context()),
		Status:     "ok",
		Categories: categories,
	}

	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) IrCategoryBrandsHandler(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &model.UnknownUserError{})
		return
	}

	// simple user check
	skillID := middleware.GetSkillID(r.Context())
	tuyaUserID, err := s.db.GetTuyaUserID(r.Context(), user.ID, skillID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get Tuya user for uid %d. Reason: %s", user.ID, err.Error())
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	// Check user rights on device
	deviceID := chi.URLParam(r, "device_id") // external (tuya) IR device id

	userDevices, err := s.getUserDevices(r.Context(), tuyaUserID, skillID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get Tuya devices for uid %d. Reason: %s", user.ID, err.Error())
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	if _, ok := userDevices[deviceID]; !ok {
		ctxlog.Warnf(r.Context(), s.Logger, "User %d has no rights to control device %s", user.ID, deviceID)
		s.render.RenderMobileError(r.Context(), w, provider.ErrorDeviceNotFound)
		return
	}

	categoryID := chi.URLParam(r, "category_id")

	brandsMap, err := s.tuyaClient.GetIrCategoryBrands(r.Context(), deviceID, categoryID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get remote category %s brands for IR device with id %s. Reason: %s", categoryID, deviceID, err.Error())
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	brands := make([]mobile.IRBrand, 0, len(brandsMap))
	for id, name := range brandsMap {
		brands = append(brands, mobile.IRBrand{ID: id, Name: name})
	}
	sort.Sort(mobile.IRBrandsByName(brands))

	response := mobile.IRCategoryBrandsResponse{
		RequestID: requestid.GetRequestID(r.Context()),
		Status:    "ok",
		Brands:    brands,
	}

	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) IrCategoryBrandPresetsHandler(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	// simple user check
	skillID := middleware.GetSkillID(r.Context())
	tuyaUserID, err := s.db.GetTuyaUserID(r.Context(), user.ID, skillID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get Tuya user for uid %d. Reason: %s", user.ID, err.Error())
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	// Check user rights on device
	deviceID := chi.URLParam(r, "device_id") // external (tuya) IR device id

	userDevices, err := s.getUserDevices(r.Context(), tuyaUserID, skillID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get Tuya devices for uid %d. Reason: %s", user.ID, err.Error())
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	if _, ok := userDevices[deviceID]; !ok {
		ctxlog.Warnf(r.Context(), s.Logger, "User %d has no rights to control device %s", user.ID, deviceID)
		s.render.RenderMobileError(r.Context(), w, provider.ErrorDeviceNotFound)
		return
	}

	categoryID := chi.URLParam(r, "category_id")
	brandID := chi.URLParam(r, "brand_id")

	presets, err := s.tuyaClient.GetIrCategoryBrandPresets(r.Context(), deviceID, categoryID, brandID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get remote control presets for brand id %s from category id %s for IR device with id %s. Reason: %s", brandID, categoryID, deviceID, err.Error())
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	response := mobile.IRCategoryBrandPresetsResponse{
		RequestID: requestid.GetRequestID(r.Context()),
		Status:    "ok",
		Presets:   presets,
	}

	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) IrRemoteControlFromPresetHandler(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	// simple user check
	skillID := middleware.GetSkillID(r.Context())
	tuyaUserID, err := s.db.GetTuyaUserID(r.Context(), user.ID, skillID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get Tuya user for uid %d. Reason: %s", user.ID, err.Error())
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	// Check user rights on device
	deviceID := chi.URLParam(r, "device_id") // external (tuya) IR device id

	userDevices, err := s.getUserDevices(r.Context(), tuyaUserID, skillID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get Tuya devices for uid %d. Reason: %s", user.ID, err.Error())
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	if _, ok := userDevices[deviceID]; !ok {
		ctxlog.Warnf(r.Context(), s.Logger, "User %d has no rights to control device %s", user.ID, deviceID)
		s.render.RenderMobileError(r.Context(), w, provider.ErrorDeviceNotFound)
		return
	}

	categoryID := tuya.IrCategoryID(chi.URLParam(r, "category_id"))
	brandID := chi.URLParam(r, "brand_id")
	presetID := chi.URLParam(r, "preset_id")

	var deviceType model.DeviceType
	if mapDeviceType, found := tuya.IRCategoriesToDeviceTypeMap[categoryID]; found {
		deviceType = mapDeviceType
	} else {
		ctxlog.Warnf(r.Context(), s.Logger, "Unknown category id %s for IR device with id %s", categoryID, deviceID)
		s.render.RenderMobileError(r.Context(), w, &tuya.ErrUnknownIRCategory{Category: tuya.IrCategoryID(categoryID)})
		return
	}

	presetKeys, err := s.tuyaClient.GetIrCategoryBrandPresetKeysMap(r.Context(), deviceID, categoryID, brandID, presetID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get remote control preset keys for brand id %s from category id %s for IR device with id %s. Reason: %s", brandID, categoryID, deviceID, err.Error())
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	response := mobile.IRCategoryBrandPresetControlResponse{
		RequestID: requestid.GetRequestID(r.Context()),
		Status:    "ok",
	}

	switch deviceType {
	case model.TvDeviceDeviceType, model.ReceiverDeviceType, model.TvBoxDeviceType:
		allowedKeys := tuya.FilterTvKnownKeys(presetKeys)
		allowedKeys = tuya.ReplaceInputSourceIRKeys(allowedKeys)
		response.Control = mobile.InfraredTVControl{
			Type: deviceType,
			Keys: allowedKeys,
		}
	case model.AcDeviceType:
		var acControlAbilities tuya.InfraredACControlAbilities
		acControlAbilities.FromKeysMap(presetKeys)
		control := mobile.InfraredACControl{Type: model.AcDeviceType}
		control.FromInfraredACControlAbilities(acControlAbilities)

		response.Control = control
	}

	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) IrCommandHandler(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	// simple user check
	skillID := middleware.GetSkillID(r.Context())
	tuyaUserID, err := s.db.GetTuyaUserID(r.Context(), user.ID, skillID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get Tuya user for uid %d. Reason: %s", user.ID, err.Error())
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(r.Context(), s.Logger, "Error reading body: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}
	defer func() { _ = r.Body.Close() }()

	ctxlog.Infof(r.Context(), s.Logger, "Get raw request from search app: %s", string(body))

	// Check user rights on device
	deviceID := chi.URLParam(r, "device_id") // external (tuya) IR device id

	userDevices, err := s.getUserDevices(r.Context(), tuyaUserID, skillID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get Tuya devices for uid %d. Reason: %s", user.ID, err.Error())
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	if _, ok := userDevices[deviceID]; !ok {
		ctxlog.Warnf(r.Context(), s.Logger, "User %d has no rights to control device %s", user.ID, deviceID)
		s.render.RenderMobileError(r.Context(), w, provider.ErrorDeviceNotFound)
		return
	}

	controlType := r.URL.Query().Get("type")
	if len(controlType) == 0 {
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	switch controlType {
	case "tv", "media": // TODO: delete 'tv' after mobile support media category:
		var command tuya.IRCommand
		err = json.Unmarshal(body, &command)
		if err != nil {
			ctxlog.Warnf(r.Context(), s.Logger, "unable to parse json body: %v", err)
			s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
			return
		}
		if err := s.tuyaClient.SendIRCommand(r.Context(), deviceID, command.PresetID, command.KeyID); err != nil {
			ctxlog.Warnf(r.Context(), s.Logger, "failed to send command to ir transmitter: %s", err)
			s.render.RenderMobileError(r.Context(), w, err)
			return
		}
	case "ac":
		var command mobile.IrAcAction
		err = json.Unmarshal(body, &command)
		if err != nil {
			ctxlog.Warnf(r.Context(), s.Logger, "unable to parse json body: %v", err)
			s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
			return
		}

		payload, err := command.ToAcStateView()
		if err != nil {
			ctxlog.Warnf(r.Context(), s.Logger, "mobile ac commands failed to convert to tuya ac send keys commands: %s", err)
			s.render.RenderMobileError(r.Context(), w, err)
			return
		}
		if err := s.tuyaClient.SendIRACCommand(r.Context(), deviceID, payload); err != nil {
			ctxlog.Warnf(r.Context(), s.Logger, "failed to send command to ir transmitter: %s", err)
			s.render.RenderMobileError(r.Context(), w, err)
			return
		}
	default:
		ctxlog.Warnf(r.Context(), s.Logger, "unsupported control device type: %s", controlType)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	s.render.RenderMobileOk(r.Context(), w)
}

func (s *Server) IrAddControlHandler(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	// simple user check
	skillID := middleware.GetSkillID(r.Context())
	tuyaUserID, err := s.db.GetTuyaUserID(r.Context(), user.ID, skillID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get Tuya user for uid %d. Reason: %s", user.ID, err.Error())
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(r.Context(), s.Logger, "Error reading body: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}
	defer func() { _ = r.Body.Close() }()

	// Check user rights on device
	deviceID := chi.URLParam(r, "device_id") // external (tuya) IR device id

	userDevices, err := s.getUserDevices(r.Context(), tuyaUserID, skillID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get Tuya devices for uid %d. Reason: %s", user.ID, err.Error())
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	if _, ok := userDevices[deviceID]; !ok {
		ctxlog.Warnf(r.Context(), s.Logger, "User %d has no rights to control device %s", user.ID, deviceID)
		s.render.RenderMobileError(r.Context(), w, provider.ErrorDeviceNotFound)
		return
	}

	var payload struct {
		CategoryID string `json:"category"`
		BrandID    string `json:"brand_id"`
		PresetID   string `json:"preset_id"`
		Name       string `json:"name"`
	}
	err = json.Unmarshal(body, &payload)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "unable to parse json body: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	if defaultName, found := tuya.KnownIrCategoryNames[tuya.IrCategoryID(payload.CategoryID)]; found {
		payload.Name = string(defaultName)
	} else {
		payload.Name = fmt.Sprintf("Пульт %s-%s", payload.PresetID, payload.BrandID)
	}

	if err := s.tuyaClient.AddIRRemoteControl(r.Context(), deviceID, payload.CategoryID, payload.BrandID, payload.PresetID, payload.Name); err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to add remote control `%#v` to ir transmitter with id: %s. Reason: %s", payload, deviceID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	s.render.RenderMobileOk(r.Context(), w)
}

func (s *Server) IrGetMatchedRemotesHandler(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	// simple user check
	skillID := middleware.GetSkillID(r.Context())
	tuyaUserID, err := s.db.GetTuyaUserID(r.Context(), user.ID, skillID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get Tuya user for uid %d. Reason: %s", user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(r.Context(), s.Logger, "Error reading body: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	ctxlog.Infof(r.Context(), s.Logger, "got raw request from search app: %s", string(body))

	var payload mobile.IRMatchedRemotes

	if err = json.Unmarshal(body, &payload); err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "unable to parse json body: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	deviceID := chi.URLParam(r, "device_id") // external (tuya) IR device id
	userDevices, err := s.tuyaClient.GetUserDevices(r.Context(), tuyaUserID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get Tuya devices for uid %d. Reason: %s", user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}
	if _, ok := userDevices[deviceID]; !ok {
		ctxlog.Warnf(r.Context(), s.Logger, "User %d has no rights to control device %s", user.ID, deviceID)
		s.render.RenderMobileError(r.Context(), w, provider.ErrorDeviceNotFound)
		return
	}

	var categoryID tuya.IrCategoryID
	deviceType := payload.MatchingType
	if mapCategoryID, found := tuya.DeviceTypeToIRCategoriesMap[deviceType]; !found {
		ctxlog.Warnf(r.Context(), s.Logger, "Unknown matching type %s for IR device with id %s", deviceType, deviceID)
		s.render.RenderMobileError(r.Context(), w, &tuya.ErrUnknownMatchingType{MatchingType: deviceType})
		return
	} else {
		categoryID = mapCategoryID
	}

	code, err := s.tuyaClient.GetIRLearnedCode(r.Context(), deviceID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to get IR learning code: %v", err)
		if xerrors.Is(err, &tuya.ErrIRLearningCodeTimeout{}) {
			s.render.RenderMobileError(r.Context(), w, &tuya.ErrIRMatchingRemotesTimeout{})
		} else {
			s.render.RenderMobileError(r.Context(), w, err)
		}
		return
	}

	matchingTypeToPresets := make(map[model.DeviceType]tuya.MatchedPresets)
	if tools.Contains(string(categoryID), tuya.MediaIrCategoryIDs) {
		// parallel get matching remotes
		type presetsCategoryID struct {
			categoryID tuya.IrCategoryID
			presets    tuya.MatchedPresets
		}
		var wg sync.WaitGroup
		presetsCh := make(chan presetsCategoryID, len(tuya.MediaIrCategoryIDs))

		for _, mediaCategoryID := range tuya.MediaIrCategoryIDs {
			wg.Add(1)
			go func(ctx context.Context, deviceID string, mediaCategoryID tuya.IrCategoryID, code string, ch chan<- presetsCategoryID) {
				defer wg.Done()
				defer func() {
					if r := recover(); r != nil {
						err := xerrors.Errorf("caught panic in requesting remote presets by IR code, category %s for device %s: %v", categoryID, deviceID, r)
						ctxlog.Warn(ctx, s.Logger, fmt.Sprintf("%v", err), log.Any("stacktrace", string(debug.Stack())))
						ch <- presetsCategoryID{
							categoryID: mediaCategoryID,
							presets:    make(tuya.MatchedPresets, 0),
						}
					}
				}()
				presets, err := s.tuyaClient.GetRemotePresetsByIRCode(r.Context(), deviceID, mediaCategoryID, code)
				if err != nil {
					ctxlog.Warnf(r.Context(), s.Logger, "failed to get remote presets by IR code for category %s for device %s: %v", categoryID, deviceID, err)
					ch <- presetsCategoryID{
						categoryID: mediaCategoryID,
						presets:    make(tuya.MatchedPresets, 0),
					}
					return
				}
				ch <- presetsCategoryID{
					categoryID: mediaCategoryID,
					presets:    presets,
				}
			}(r.Context(), deviceID, tuya.IrCategoryID(mediaCategoryID), code, presetsCh)
		}
		go func() {
			wg.Wait()
			close(presetsCh)
		}()
		// collect results
		for response := range presetsCh {
			matchingTypeToPresets[tuya.IRCategoriesToDeviceTypeMap[response.categoryID]] = response.presets
		}
	} else {
		presets, err := s.tuyaClient.GetRemotePresetsByIRCode(r.Context(), deviceID, categoryID, code)
		if err != nil {
			ctxlog.Warnf(r.Context(), s.Logger, "failed to get remote presets by IR code for category %s for device %s: %v", categoryID, deviceID, err)
			s.render.RenderMobileError(r.Context(), w, err)
			return
		}
		matchingTypeToPresets[deviceType] = presets
	}

	resultPresets := make(map[model.DeviceType]tuya.MatchedPresets)
	if len(payload.TypeToMatchedPresets) == 0 {
		resultPresets = matchingTypeToPresets
	} else {
		noPresetsAfterIntersection := true
		for deviceType, presets := range matchingTypeToPresets {
			if payloadPresets, found := payload.TypeToMatchedPresets[deviceType]; found {
				resultPresets[deviceType] = presets.Intersect(payloadPresets)
				if len(resultPresets[deviceType]) > 0 {
					noPresetsAfterIntersection = false
				}
			}
		}
		// after intersection all map matched presets are empty -> return payload map
		if noPresetsAfterIntersection {
			resultPresets = payload.TypeToMatchedPresets
		}
	}

	response := mobile.IRMatchedRemotesResponse{
		Status:    "ok",
		RequestID: requestid.GetRequestID(r.Context()),
		MatchedRemotes: mobile.IRMatchedRemotes{
			MatchingType:         deviceType,
			TypeToMatchedPresets: resultPresets,
		},
	}

	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) IrSaveCustomControlHandler(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	// simple user check
	skillID := middleware.GetSkillID(r.Context())
	tuyaUserID, err := s.db.GetTuyaUserID(r.Context(), user.ID, skillID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get Tuya user for uid %d. Reason: %s", user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	userDevices, err := s.tuyaClient.GetUserDevices(r.Context(), tuyaUserID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get Tuya devices for uid %d. Reason: %s", user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	deviceID := chi.URLParam(r, "device_id") // external (tuya) IR device id
	device, ok := userDevices[deviceID]
	if !ok {
		ctxlog.Warnf(r.Context(), s.Logger, "User %d has no rights to control device %s", user.ID, deviceID)
		s.render.RenderMobileError(r.Context(), w, provider.ErrorDeviceNotFound)
		return
	}
	productID := device.ProductID

	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(r.Context(), s.Logger, "Error reading body: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	ctxlog.Infof(r.Context(), s.Logger, "got raw request from search app: %s", string(body))

	var payload mobile.IRSaveCustomControlRequest
	if err = binder.Bind(valid.NewValidationCtx(), body, &payload); err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "invalid save custom control request body: %v", err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	irCode, err := s.tuyaClient.GetIRLearnedCode(r.Context(), deviceID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to get IR learning code: %v", err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	// save in tuya api
	tuyaCode := tuya.IRCode{
		Code: irCode,
		Name: tuya.GenerateIRCodeName(),
	}

	controlID, err := s.tuyaClient.SaveIRCustomControl(r.Context(), deviceID, []tuya.IRCode{tuyaCode})
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to save custom control via tuya api: %v", err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	// save in tuya adapter
	customButton := tuya.IRCustomButton{
		Key:  tuyaCode.Name,
		Name: string(payload.Code.CustomName),
	}

	customControl := tuya.IRCustomControl{
		ID:         controlID,
		Name:       string(payload.Name),
		DeviceType: payload.DeviceType,
		Buttons:    []tuya.IRCustomButton{customButton},
	}

	if err := s.db.StoreCustomControl(r.Context(), tuyaUserID, customControl); err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to save custom control: %v", err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	if err = s.pushDiscoveryIRControls(r.Context(), user.ID, tuyaUserID, deviceID, []string{controlID}, productID); err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to push discovery custom control %s: %v", controlID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	response := mobile.IRSaveCustomControlResponse{
		Status:    "ok",
		RequestID: requestid.GetRequestID(r.Context()),
		DeviceID:  controlID,
	}
	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) IrDeleteCustomButtonFromControlHandler(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	// simple user check
	skillID := middleware.GetSkillID(r.Context())
	tuyaUserID, err := s.db.GetTuyaUserID(r.Context(), user.ID, skillID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get Tuya user for uid %d. Reason: %v", user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	userDevices, err := s.tuyaClient.GetUserDevices(r.Context(), tuyaUserID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get Tuya devices for uid %d. Reason: %v", user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	deviceID := chi.URLParam(r, "device_id") // external (tuya) IR device id
	device, ok := userDevices[deviceID]
	if !ok {
		ctxlog.Warnf(r.Context(), s.Logger, "User %d has no rights to control device %s", user.ID, deviceID)
		s.render.RenderMobileError(r.Context(), w, provider.ErrorDeviceNotFound)
		return
	}
	productID := device.ProductID

	controlID := chi.URLParam(r, "control_id") // custom control IR id
	irControl, err := s.db.SelectCustomControl(r.Context(), tuyaUserID, controlID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to get custom control %s. Reason: %v", controlID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	if _, ok := userDevices[controlID]; !ok {
		ctxlog.Warnf(r.Context(), s.Logger, "User %d has no rights to control custom control %s", user.ID, controlID)
		if err := s.db.DeleteCustomControl(r.Context(), tuyaUserID, controlID); err != nil {
			ctxlog.Warnf(r.Context(), s.Logger, "Failed to delete custom control %s for tuya user %s. Reason: %v", controlID, tuyaUserID, err)
		}
		s.render.RenderMobileError(r.Context(), w, provider.ErrorDeviceNotFound)
		return
	}

	buttonKey := chi.URLParam(r, "button_id")

	resultButtons := make([]tuya.IRCustomButton, 0)
	for _, button := range irControl.Buttons {
		if button.Key == buttonKey {
			continue
		}
		resultButtons = append(resultButtons, button)
	}
	if len(resultButtons) == 0 {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to delete custom button %s from control %s: %v", buttonKey, controlID, &tuya.ErrLastCustomButtonDeletion{})
		s.render.RenderMobileError(r.Context(), w, &tuya.ErrLastCustomButtonDeletion{})
		return
	}
	irControl.Buttons = resultButtons

	if err = s.db.StoreCustomControl(r.Context(), tuyaUserID, irControl); err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to delete custom button %s from control %s: %v", buttonKey, controlID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	if err = s.pushDiscoveryIRControls(r.Context(), user.ID, tuyaUserID, deviceID, []string{controlID}, productID); err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to push discovery custom control %s: %v", controlID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	s.render.RenderMobileOk(r.Context(), w)
}

func (s *Server) IrAddCustomButtonToControlHandler(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	// simple user check
	skillID := middleware.GetSkillID(r.Context())
	tuyaUserID, err := s.db.GetTuyaUserID(r.Context(), user.ID, skillID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get Tuya user for uid %d. Reason: %v", user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	userDevices, err := s.tuyaClient.GetUserDevices(r.Context(), tuyaUserID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get Tuya devices for uid %d. Reason: %v", user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	deviceID := chi.URLParam(r, "device_id") // external (tuya) IR device id
	device, ok := userDevices[deviceID]
	if !ok {
		ctxlog.Warnf(r.Context(), s.Logger, "User %d has no rights to control device %s", user.ID, deviceID)
		s.render.RenderMobileError(r.Context(), w, provider.ErrorDeviceNotFound)
		return
	}
	productID := device.ProductID

	controlID := chi.URLParam(r, "control_id") // custom control IR id
	irControl, err := s.db.SelectCustomControl(r.Context(), tuyaUserID, controlID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get custom control %s: %v", controlID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	if _, ok := userDevices[controlID]; !ok {
		ctxlog.Warnf(r.Context(), s.Logger, "User %d has no rights to control custom control %s", user.ID, controlID)
		if err := s.db.DeleteCustomControl(r.Context(), tuyaUserID, controlID); err != nil {
			ctxlog.Warnf(r.Context(), s.Logger, "Failed to delete custom control %s for tuya user %s. Reason: %v", controlID, tuyaUserID, err)
		}
		s.render.RenderMobileError(r.Context(), w, provider.ErrorDeviceNotFound)
		return
	}

	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(r.Context(), s.Logger, "Error reading body: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	ctxlog.Infof(r.Context(), s.Logger, "got raw request from search app: %s", string(body))

	vctx := valid.NewValidationCtx()
	vctx.Add(mobile.IRCustomButtonExistingNameValidator, mobile.IRCustomButtonExistingNameValidatorFunc(irControl.Buttons))
	var payload mobile.IRCustomCode
	if err = binder.Bind(vctx, body, &payload); err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "invalid custom button name: %v", err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	irCode, err := s.tuyaClient.GetIRLearnedCode(r.Context(), deviceID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to get IR learning code: %v", err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	// save in tuya api
	tuyaCode := tuya.IRCode{
		Code: irCode,
		Name: tuya.GenerateIRCodeName(),
	}

	// check on if we already have button with the same key
	// possibility is very low but to prevent issues
	for _, button := range irControl.Buttons {
		if button.Key == tuyaCode.Name {
			tuyaCode.Name = tuya.GenerateIRCodeName()
			break
		}
	}

	if err = s.tuyaClient.UpdateIRCustomControl(r.Context(), deviceID, controlID, []tuya.IRCode{tuyaCode}); err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to update custom control %s via tuya api: %v", controlID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	// save in tuya adapter
	customButton := tuya.IRCustomButton{
		Key:  tuyaCode.Name,
		Name: string(payload.CustomName),
	}
	irControl.Buttons = append(irControl.Buttons, customButton)

	if err = s.db.StoreCustomControl(r.Context(), tuyaUserID, irControl); err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to update custom control %s: %v", controlID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	if err = s.pushDiscoveryIRControls(r.Context(), user.ID, tuyaUserID, deviceID, []string{controlID}, productID); err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to push discovery custom control %s: %v", controlID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	s.render.RenderMobileOk(r.Context(), w)
}

func (s *Server) IrRenameCustomButtonInControlHandler(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	// simple user check
	skillID := middleware.GetSkillID(r.Context())
	tuyaUserID, err := s.db.GetTuyaUserID(r.Context(), user.ID, skillID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get Tuya user for uid %d. Reason: %v", user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	userDevices, err := s.tuyaClient.GetUserDevices(r.Context(), tuyaUserID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get Tuya devices for uid %d. Reason: %v", user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	deviceID := chi.URLParam(r, "device_id") // external (tuya) IR device id
	device, ok := userDevices[deviceID]
	if !ok {
		ctxlog.Warnf(r.Context(), s.Logger, "User %d has no rights to control device %s", user.ID, deviceID)
		s.render.RenderMobileError(r.Context(), w, provider.ErrorDeviceNotFound)
		return
	}
	productID := device.ProductID

	controlID := chi.URLParam(r, "control_id") // custom control IR id
	irControl, err := s.db.SelectCustomControl(r.Context(), tuyaUserID, controlID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to get custom control %s. Reason: %v", controlID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	if _, ok := userDevices[controlID]; !ok {
		ctxlog.Warnf(r.Context(), s.Logger, "User %d has no rights to control custom control %s", user.ID, controlID)
		if err := s.db.DeleteCustomControl(r.Context(), tuyaUserID, controlID); err != nil {
			ctxlog.Warnf(r.Context(), s.Logger, "Failed to delete custom control %s for tuya user %s. Reason: %v", controlID, tuyaUserID, err)
		}
		s.render.RenderMobileError(r.Context(), w, provider.ErrorDeviceNotFound)
		return
	}

	buttonKey := chi.URLParam(r, "button_id")

	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(r.Context(), s.Logger, "Error reading body: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	ctxlog.Infof(r.Context(), s.Logger, "got raw request from search app: %s", string(body))

	// check if button exist and make validation context
	buttonExist := false
	validationButtons := make([]tuya.IRCustomButton, 0, len(irControl.Buttons))
	var beingRenamedButton tuya.IRCustomButton
	for _, button := range irControl.Buttons {
		if button.Key != buttonKey {
			validationButtons = append(validationButtons, button)
		} else {
			buttonExist = true
			beingRenamedButton = button
		}
	}
	if !buttonExist {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to rename custom button %s in control %s: %v", buttonKey, controlID, &tuya.ErrCustomButtonNotFound{})
		s.render.RenderMobileError(r.Context(), w, &tuya.ErrCustomButtonNotFound{})
		return
	}

	vctx := valid.NewValidationCtx()
	vctx.Add(mobile.IRCustomButtonExistingNameValidator, mobile.IRCustomButtonExistingNameValidatorFunc(validationButtons))
	var payload mobile.IRCustomButtonRenameRequest
	if err = binder.Bind(vctx, body, &payload); err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "invalid custom button name: %v", err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	// rename and storing
	beingRenamedButton.Name = payload.Name
	validationButtons = append(validationButtons, beingRenamedButton)
	irControl.Buttons = validationButtons

	if err = s.db.StoreCustomControl(r.Context(), tuyaUserID, irControl); err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to rename custom button %s from control %s: %v", buttonKey, controlID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	if err = s.pushDiscoveryIRControls(r.Context(), user.ID, tuyaUserID, deviceID, []string{controlID}, productID); err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to push discovery custom control %s: %v", controlID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	s.render.RenderMobileOk(r.Context(), w)
}

func (s *Server) IrValidateCustomButtonNameAcrossOtherButtonsHandler(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	// simple user check
	skillID := middleware.GetSkillID(r.Context())
	tuyaUserID, err := s.db.GetTuyaUserID(r.Context(), user.ID, skillID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get Tuya user for uid %d. Reason: %v", user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	controlID := chi.URLParam(r, "control_id") // custom control IR id
	irControl, err := s.db.SelectCustomControl(r.Context(), tuyaUserID, controlID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get custom control %s: %v", controlID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(r.Context(), s.Logger, "Error reading body: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	ctxlog.Infof(r.Context(), s.Logger, "got raw request from search app: %s", string(body))

	vctx := valid.NewValidationCtx()
	vctx.Add(mobile.IRCustomButtonExistingNameValidator, mobile.IRCustomButtonExistingNameValidatorFunc(irControl.Buttons))
	var payload mobile.IRCustomButtonValidationRequest
	if err = binder.Bind(vctx, body, &payload); err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "invalid custom button name: %v", err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	s.render.RenderMobileOk(r.Context(), w)
}

func (s *Server) IrValidateCustomButtonNameHandler(w http.ResponseWriter, r *http.Request) {
	_, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(r.Context(), s.Logger, "Error reading body: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	ctxlog.Infof(r.Context(), s.Logger, "got raw request from search app: %s", string(body))

	var payload mobile.IRCustomButtonValidationRequest
	if err = binder.Bind(valid.NewValidationCtx(), body, &payload); err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "invalid custom button name: %v", err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	s.render.RenderMobileOk(r.Context(), w)
}

func (s *Server) IrValidateCustomControlNameHandler(w http.ResponseWriter, r *http.Request) {
	_, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(r.Context(), s.Logger, "Error reading body: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	ctxlog.Infof(r.Context(), s.Logger, "got raw request from search app: %s", string(body))

	var payload mobile.IRCustomControlNameValidationRequest
	if err = binder.Bind(valid.NewValidationCtx(), body, &payload); err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "invalid custom control name: %v", err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	s.render.RenderMobileOk(r.Context(), w)
}

func (s *Server) IrCustomControlConfigurationHandler(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	// simple user check
	skillID := middleware.GetSkillID(r.Context())
	tuyaUserID, err := s.db.GetTuyaUserID(r.Context(), user.ID, skillID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get Tuya user for uid %d. Reason: %v", user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	userDevices, err := s.tuyaClient.GetUserDevices(r.Context(), tuyaUserID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get Tuya devices for uid %d. Reason: %v", user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	deviceID := chi.URLParam(r, "device_id") // external (tuya) IR device id
	if _, ok := userDevices[deviceID]; !ok {
		ctxlog.Warnf(r.Context(), s.Logger, "User %d has no rights to control device %s", user.ID, deviceID)
		s.render.RenderMobileError(r.Context(), w, provider.ErrorDeviceNotFound)
		return
	}

	controlID := chi.URLParam(r, "control_id") // custom control IR id
	irControl, err := s.db.SelectCustomControl(r.Context(), tuyaUserID, controlID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get custom control %s for uid %d. Reason: %v", controlID, user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	if _, ok := userDevices[controlID]; !ok {
		ctxlog.Warnf(r.Context(), s.Logger, "User %d has no rights to control custom control %s", user.ID, controlID)
		if err := s.db.DeleteCustomControl(r.Context(), tuyaUserID, controlID); err != nil {
			ctxlog.Warnf(r.Context(), s.Logger, "Failed to delete custom control %s for tuya user %s. Reason: %v", controlID, tuyaUserID, err)
		}
		s.render.RenderMobileError(r.Context(), w, provider.ErrorDeviceNotFound)
		return
	}

	var configurationView mobile.IRCustomControlConfigurationView
	configurationView.FromIRCustomControl(irControl)

	s.render.RenderJSON(r.Context(), w, configurationView)
}

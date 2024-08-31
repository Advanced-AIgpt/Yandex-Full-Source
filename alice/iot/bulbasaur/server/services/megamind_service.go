package services

import (
	"context"
	"fmt"
	"math/rand"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/experiments"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/settings"
	"a.yandex-team.ru/alice/iot/bulbasaur/logging"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	"a.yandex-team.ru/alice/library/go/random"
	"a.yandex-team.ru/alice/library/go/setrace"
	megamindcommonpb "a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

type MegamindService struct {
	logger             log.Logger
	settingsController settings.IController
	megamind           *libmegamind.Dispatcher
}

func NewMegamindService(logger log.Logger, settingsController settings.IController, megamind *libmegamind.Dispatcher) *MegamindService {
	return &MegamindService{
		logger:             logger,
		settingsController: settingsController,
		megamind:           megamind,
	}
}

func (s *MegamindService) HandleRunRequest(ctx context.Context, runRequest *scenarios.TScenarioRunRequest) (*scenarios.TScenarioRunResponse, error) {
	ctx = random.ContextWithRand(ctx, rand.New(rand.NewSource(int64(runRequest.BaseRequest.RandomSeed))))
	userSettings, err := s.settingsController.ExtractUserSettingsFromRequest(ctx, runRequest.BaseRequest)
	if err != nil {
		ctxlog.Warnf(ctx, s.logger, "failed to extract settings from request memento data: %v", err)
		setrace.InfoLogEvent(
			ctx, s.logger,
			fmt.Sprintf("failed to extract settings from request memento data: %v", err))
	} else {
		setrace.InfoLogEvent(
			ctx, s.logger,
			"User settings",
			log.Any("user_settings", userSettings),
		)
	}
	ctx = settings.ContextWithSettings(ctx, userSettings)
	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warnf(ctx, s.logger, "failed to get user from context while handling run request: %v", err)
		user = model.User{}
	} else if dataSource, dataSourceOk := runRequest.DataSources[int32(megamindcommonpb.EDataSourceType_BLACK_BOX)]; dataSourceOk {
		userInfo := libmegamind.NewUserInfo(dataSource.GetUserInfo())
		if expManager, err := experiments.ManagerFromContext(ctx); err != nil {
			msg := fmt.Sprintf("failed to get experiments manager: %v", err)
			ctxlog.Warn(ctx, s.logger, msg)
			setrace.ErrorLogEvent(ctx, s.logger, msg)
		} else {
			expManager.SetStaff(user.ID, userInfo.IsStaff)
		}
	}
	userField := logging.UserField("user", user)
	ctxlog.Info(ctx, s.logger, "UserInfo", userField)
	setrace.InfoLogEvent(ctx, s.logger, "UserInfo", userField)

	ctxlog.Info(ctx, s.logger, "megamind_run_request", logging.ProtoJSON("body", runRequest))
	setrace.InfoLogEvent(ctx, s.logger, "megamind_run_request", libmegamind.RunRequestLogView("run_request", runRequest))
	response, err := s.megamind.Run(ctx, runRequest, user)
	if err != nil {
		ctxlog.Warnf(ctx, s.logger, "failed to perform megamind run: %v", err)
		setrace.ErrorLogEvent(ctx, s.logger,
			"run response error",
			log.String("error", err.Error()),
		)
		return nil, err
	}
	applyArgumentsFieldJSON := logging.ProtoJSON("apply_arguments", response.GetApplyArguments())
	ctxlog.Info(ctx, s.logger,
		"run response success",
		log.String("scenario", response.GetResponseBody().GetAnalyticsInfo().GetProductScenarioName()),
		log.String("intent", response.GetResponseBody().GetAnalyticsInfo().GetIntent()),
		log.Bool("irrelevant", response.GetFeatures().GetIsIrrelevant()),
		applyArgumentsFieldJSON,
	)
	setrace.InfoLogEvent(ctx, s.logger,
		"run response success",
		log.String("scenario", response.GetResponseBody().GetAnalyticsInfo().GetProductScenarioName()),
		log.String("intent", response.GetResponseBody().GetAnalyticsInfo().GetIntent()),
		log.Bool("irrelevant", response.GetFeatures().GetIsIrrelevant()),
		applyArgumentsFieldJSON,
	)
	return response, nil
}

func (s *MegamindService) HandleApplyRequest(ctx context.Context, user model.User, applyRequest *scenarios.TScenarioApplyRequest) (*scenarios.TScenarioApplyResponse, error) {
	ctx = random.ContextWithRand(ctx, rand.New(rand.NewSource(int64(applyRequest.BaseRequest.RandomSeed))))
	userSettings, err := s.settingsController.ExtractUserSettingsFromRequest(ctx, applyRequest.BaseRequest)
	if err != nil {
		ctxlog.Warnf(ctx, s.logger, "failed to extract settings from request memento data: %v", err)
		setrace.InfoLogEvent(
			ctx, s.logger,
			fmt.Sprintf("failed to extract settings from request memento data: %v", err))
	} else {
		setrace.InfoLogEvent(
			ctx, s.logger,
			"User settings",
			log.Any("user_settings", userSettings),
		)
	}
	ctx = settings.ContextWithSettings(ctx, userSettings)
	if dataSource, dataSourceOk := applyRequest.DataSources[int32(megamindcommonpb.EDataSourceType_BLACK_BOX)]; dataSourceOk {
		userInfo := libmegamind.NewUserInfo(dataSource.GetUserInfo())
		expManager, err := experiments.ManagerFromContext(ctx)
		if err != nil {
			msg := fmt.Sprintf("failed to get experiments manager: %v", err)
			ctxlog.Warn(ctx, s.logger, msg)
			setrace.ErrorLogEvent(ctx, s.logger, msg)
		} else {
			expManager.SetStaff(user.ID, userInfo.IsStaff)
		}
	}
	userField := logging.UserField("user", user)
	ctxlog.Info(ctx, s.logger, "UserInfo", userField)
	setrace.InfoLogEvent(ctx, s.logger, "UserInfo", userField)

	applyRequestJSON := logging.ProtoJSON("body", applyRequest)
	ctxlog.Info(ctx, s.logger, "megamind_apply_request", applyRequestJSON)
	setrace.InfoLogEvent(ctx, s.logger, "megamind_apply_request", libmegamind.ApplyRequestLogView("apply_request", applyRequest))
	response, err := s.megamind.Apply(ctx, applyRequest, user)
	if err != nil {
		ctxlog.Warnf(ctx, s.logger, "failed to perform megamind apply: %v", err)
		setrace.ErrorLogEvent(ctx, s.logger,
			"apply response error",
			log.String("error", err.Error()),
		)
		return nil, err
	}
	layoutFieldJSON := logging.ProtoJSON("layout", response.GetResponseBody().GetLayout())
	ctxlog.Info(ctx, s.logger,
		"apply response success",
		log.String("scenario", response.GetResponseBody().GetAnalyticsInfo().GetProductScenarioName()),
		log.String("intent", response.GetResponseBody().GetAnalyticsInfo().GetIntent()),
		layoutFieldJSON,
	)
	setrace.InfoLogEvent(ctx, s.logger,
		"apply response success",
		log.String("scenario", response.GetResponseBody().GetAnalyticsInfo().GetProductScenarioName()),
		log.String("intent", response.GetResponseBody().GetAnalyticsInfo().GetIntent()),
		layoutFieldJSON,
	)
	return response, nil
}

func (s *MegamindService) HandleContinueRequest(ctx context.Context, user model.User, continueRequest *scenarios.TScenarioApplyRequest) (*scenarios.TScenarioContinueResponse, error) {
	ctx = random.ContextWithRand(ctx, rand.New(rand.NewSource(int64(continueRequest.BaseRequest.RandomSeed))))
	userSettings, err := s.settingsController.ExtractUserSettingsFromRequest(ctx, continueRequest.BaseRequest)
	if err != nil {
		ctxlog.Warnf(ctx, s.logger, "failed to extract settings from request memento data: %v", err)
		setrace.InfoLogEvent(
			ctx, s.logger,
			fmt.Sprintf("failed to extract settings from request memento data: %v", err))
	} else {
		setrace.InfoLogEvent(
			ctx, s.logger,
			"User settings",
			log.Any("user_settings", userSettings),
		)
	}
	ctx = settings.ContextWithSettings(ctx, userSettings)
	if dataSource, dataSourceOk := continueRequest.DataSources[int32(megamindcommonpb.EDataSourceType_BLACK_BOX)]; dataSourceOk {
		userInfo := libmegamind.NewUserInfo(dataSource.GetUserInfo())
		expManager, err := experiments.ManagerFromContext(ctx)
		if err != nil {
			msg := fmt.Sprintf("failed to get experiments manager: %v", err)
			ctxlog.Warn(ctx, s.logger, msg)
			setrace.ErrorLogEvent(ctx, s.logger, msg)
		} else {
			expManager.SetStaff(user.ID, userInfo.IsStaff)
		}
	}
	userField := logging.UserField("user", user)
	ctxlog.Info(ctx, s.logger, "UserInfo", userField)
	setrace.InfoLogEvent(ctx, s.logger, "UserInfo", userField)

	continueRequestJSON := logging.ProtoJSON("body", continueRequest)
	ctxlog.Info(ctx, s.logger, "megamind_continue_request", continueRequestJSON)
	setrace.InfoLogEvent(ctx, s.logger, "megamind_continue_request", libmegamind.ApplyRequestLogView("continue_request", continueRequest))
	response, err := s.megamind.Continue(ctx, continueRequest, user)
	if err != nil {
		ctxlog.Warnf(ctx, s.logger, "failed to perform megamind continue: %v", err)
		setrace.ErrorLogEvent(ctx, s.logger,
			"continue response error",
			log.String("error", err.Error()),
		)
		return nil, err
	}
	layoutFieldJSON := logging.ProtoJSON("layout", response.GetResponseBody().GetLayout())
	ctxlog.Info(ctx, s.logger,
		"continue response success",
		log.String("scenario", response.GetResponseBody().GetAnalyticsInfo().GetProductScenarioName()),
		log.String("intent", response.GetResponseBody().GetAnalyticsInfo().GetIntent()),
		layoutFieldJSON,
	)
	setrace.InfoLogEvent(ctx, s.logger,
		"continue response success",
		log.String("scenario", response.GetResponseBody().GetAnalyticsInfo().GetProductScenarioName()),
		log.String("intent", response.GetResponseBody().GetAnalyticsInfo().GetIntent()),
		layoutFieldJSON,
	)
	return response, nil
}

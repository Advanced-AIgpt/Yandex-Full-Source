package server

import (
	"fmt"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/setrace"
	"a.yandex-team.ru/alice/megamind/protos/blackbox"
	megamindcommonpb "a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/apphost/api/service/go/apphost"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var megamindRequestItemType = "mm_scenario_request"
var megamindResponseItemType = "mm_scenario_response"

func (s *Server) getMegamindDataSourcesFromApphost(apphostCtx apphost.Context) (map[int32]*scenarios.TDataSource, error) {
	ctx := apphostCtx.Context()
	dataSources := make(map[int32]*scenarios.TDataSource)

	blackboxDataSourceKey := "datasource_BLACK_BOX"
	var blackboxDataSource blackbox.TBlackBoxUserInfo
	if err := apphostCtx.GetOnePB(blackboxDataSourceKey, &blackboxDataSource); err == nil {
		dataSources[int32(megamindcommonpb.EDataSourceType_BLACK_BOX)] = &scenarios.TDataSource{
			Type: &scenarios.TDataSource_UserInfo{
				UserInfo: &blackboxDataSource,
			},
		}
	} else {
		var missingItemErr apphost.MissingItemError
		if !xerrors.As(err, &missingItemErr) {
			msg := fmt.Sprintf("failed to read %s datasource: %v", blackboxDataSourceKey, err)
			ctxlog.Info(ctx, s.Logger, msg)
			setrace.InfoLogEvent(ctx, s.Logger, msg)
			return nil, err
		}
	}

	begemotDataSourceKey := "datasource_BEGEMOT_IOT_NLU_RESULT"
	var begemotDataSource scenarios.TBegemotIotNluResult
	if err := apphostCtx.GetOnePB(begemotDataSourceKey, &begemotDataSource); err == nil {
		dataSources[int32(megamindcommonpb.EDataSourceType_BEGEMOT_IOT_NLU_RESULT)] = &scenarios.TDataSource{
			Type: &scenarios.TDataSource_BegemotIotNluResult{
				BegemotIotNluResult: &begemotDataSource,
			},
		}
	} else {
		var missingItemErr apphost.MissingItemError
		if !xerrors.As(err, &missingItemErr) {
			msg := fmt.Sprintf("failed to read %s datasource: %v", begemotDataSourceKey, err)
			ctxlog.Info(ctx, s.Logger, msg)
			setrace.InfoLogEvent(ctx, s.Logger, msg)
			return nil, err
		}
	}

	iotDataSourceKey := "datasource_IOT_USER_INFO"
	var iotDataSource megamindcommonpb.TIoTUserInfo
	if err := apphostCtx.GetOnePB(iotDataSourceKey, &iotDataSource); err == nil {
		dataSources[int32(megamindcommonpb.EDataSourceType_IOT_USER_INFO)] = &scenarios.TDataSource{
			Type: &scenarios.TDataSource_IoTUserInfo{
				IoTUserInfo: &iotDataSource,
			},
		}
	} else {
		var missingItemErr apphost.MissingItemError
		if !xerrors.As(err, &missingItemErr) {
			msg := fmt.Sprintf("failed to read %s datasource: %v", iotDataSourceKey, err)
			ctxlog.Info(ctx, s.Logger, msg)
			setrace.InfoLogEvent(ctx, s.Logger, msg)
			return nil, err
		}
	}

	tandemDataSourceKey := "datasource_TANDEM_ENVIRONMENT_STATE"
	var tandemDataSource megamindcommonpb.TTandemEnvironmentState
	if err := apphostCtx.GetOnePB(tandemDataSourceKey, &tandemDataSource); err == nil {
		dataSources[int32(megamindcommonpb.EDataSourceType_TANDEM_ENVIRONMENT_STATE)] = &scenarios.TDataSource{
			Type: &scenarios.TDataSource_TandemEnvironmentState{
				TandemEnvironmentState: &tandemDataSource,
			},
		}
	} else {
		var missingItemErr apphost.MissingItemError
		if !xerrors.As(err, &missingItemErr) {
			msg := fmt.Sprintf("failed to read %s datasource: %v", tandemDataSourceKey, err)
			ctxlog.Info(ctx, s.Logger, msg)
			setrace.InfoLogEvent(ctx, s.Logger, msg)
			return nil, err
		}
	}

	environmentStateDataSourceKey := "datasource_ENVIRONMENT_STATE"
	var environmentStateDataSource megamindcommonpb.TEnvironmentState
	if err := apphostCtx.GetOnePB(environmentStateDataSourceKey, &environmentStateDataSource); err == nil {
		dataSources[int32(megamindcommonpb.EDataSourceType_ENVIRONMENT_STATE)] = &scenarios.TDataSource{
			Type: &scenarios.TDataSource_EnvironmentState{
				EnvironmentState: &environmentStateDataSource,
			},
		}
	} else {
		var missingItemErr apphost.MissingItemError
		if !xerrors.As(err, &missingItemErr) {
			msg := fmt.Sprintf("failed to read %s datasource: %v", environmentStateDataSourceKey, err)
			ctxlog.Info(ctx, s.Logger, msg)
			setrace.InfoLogEvent(ctx, s.Logger, msg)
			return nil, err
		}
	}
	return dataSources, nil
}

func (s *Server) apphostMegamindRunHandler(apphostCtx apphost.Context) error {
	ctx := apphostCtx.Context()

	var runRequest scenarios.TScenarioRunRequest
	if err := apphostCtx.GetOnePB(megamindRequestItemType, &runRequest); err != nil {
		msg := fmt.Sprintf("failed to read run request: %v", err)
		ctxlog.Info(ctx, s.Logger, msg)
		setrace.InfoLogEvent(ctx, s.Logger, msg)
		return apphost.RequestErrorf(msg)
	}

	// dataSources are not filled in apphost, we need to read them manually
	dataSources, err := s.getMegamindDataSourcesFromApphost(apphostCtx)
	if err != nil {
		msg := fmt.Sprintf("failed to get datasources: %v", err)
		ctxlog.Info(ctx, s.Logger, msg)
		setrace.InfoLogEvent(ctx, s.Logger, msg)
		return apphost.RequestErrorf(msg)
	}
	runRequest.DataSources = dataSources

	runResponse, err := s.megamindService.HandleRunRequest(ctx, &runRequest)
	if err != nil {
		msg := fmt.Sprintf("failed to handle run request: %v", err)
		ctxlog.Info(ctx, s.Logger, msg)
		setrace.InfoLogEvent(ctx, s.Logger, msg)
		return apphost.RequestErrorf(msg)
	}
	return apphostCtx.AddPB(megamindResponseItemType, runResponse)
}

func (s *Server) apphostMegamindApplyHandler(apphostCtx apphost.Context) error {
	ctx := apphostCtx.Context()

	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		msg := fmt.Sprintf("cannot authorize user: %v", err)
		ctxlog.Warn(ctx, s.Logger, msg)
		setrace.ErrorLogEvent(ctx, s.Logger, msg)
		return apphost.RequestErrorf(msg)
	}

	var applyRequest scenarios.TScenarioApplyRequest
	if err := apphostCtx.GetOnePB(megamindRequestItemType, &applyRequest); err != nil {
		msg := fmt.Sprintf("failed to read run request: %v", err)
		ctxlog.Info(ctx, s.Logger, msg)
		setrace.InfoLogEvent(ctx, s.Logger, msg)
		return apphost.RequestErrorf(msg)
	}

	// dataSources are not filled in apphost, we need to read them manually
	dataSources, err := s.getMegamindDataSourcesFromApphost(apphostCtx)
	if err != nil {
		msg := fmt.Sprintf("failed to get datasources: %v", err)
		ctxlog.Info(ctx, s.Logger, msg)
		setrace.InfoLogEvent(ctx, s.Logger, msg)
		return apphost.RequestErrorf(msg)
	}
	applyRequest.DataSources = dataSources

	runResponse, err := s.megamindService.HandleApplyRequest(ctx, user, &applyRequest)
	if err != nil {
		msg := fmt.Sprintf("failed to handle apply request: %v", err)
		ctxlog.Info(ctx, s.Logger, msg)
		setrace.InfoLogEvent(ctx, s.Logger, msg)
		return apphost.RequestErrorf(msg)
	}
	return apphostCtx.AddPB(megamindResponseItemType, runResponse)
}

func (s *Server) apphostMegamindContinueHandler(apphostCtx apphost.Context) error {
	ctx := apphostCtx.Context()

	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		msg := fmt.Sprintf("cannot authorize user: %v", err)
		ctxlog.Warn(ctx, s.Logger, msg)
		setrace.ErrorLogEvent(ctx, s.Logger, msg)
		return apphost.RequestErrorf(msg)
	}

	var continueRequest scenarios.TScenarioApplyRequest
	if err := apphostCtx.GetOnePB(megamindRequestItemType, &continueRequest); err != nil {
		msg := fmt.Sprintf("failed to read continue request: %v", err)
		ctxlog.Info(ctx, s.Logger, msg)
		setrace.InfoLogEvent(ctx, s.Logger, msg)
		return apphost.RequestErrorf(msg)
	}

	// dataSources are not filled in apphost, we need to read them manually
	dataSources, err := s.getMegamindDataSourcesFromApphost(apphostCtx)
	if err != nil {
		msg := fmt.Sprintf("failed to get datasources: %v", err)
		ctxlog.Info(ctx, s.Logger, msg)
		setrace.InfoLogEvent(ctx, s.Logger, msg)
		return apphost.RequestErrorf(msg)
	}
	continueRequest.DataSources = dataSources

	runResponse, err := s.megamindService.HandleApplyRequest(ctx, user, &continueRequest)
	if err != nil {
		msg := fmt.Sprintf("failed to handle apply request: %v", err)
		ctxlog.Info(ctx, s.Logger, msg)
		setrace.InfoLogEvent(ctx, s.Logger, msg)
		return apphost.RequestErrorf(msg)
	}
	return apphostCtx.AddPB(megamindResponseItemType, runResponse)
}

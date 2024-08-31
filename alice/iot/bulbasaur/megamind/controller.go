package megamind

import (
	"context"
	"fmt"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/repository"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/nlg"
	"a.yandex-team.ru/alice/iot/bulbasaur/protos"
	"a.yandex-team.ru/alice/library/go/alice4business"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	"a.yandex-team.ru/alice/library/go/setrace"
	"a.yandex-team.ru/alice/library/go/userctx"
	pcommon "a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Controller struct {
	logger          log.Logger
	repository      repository.IController
	runProcessors   []RunProcessor
	applyProcessors []ApplyProcessor
}

func NewController(logger log.Logger, repository repository.IController) *Controller {
	return &Controller{
		logger:          logger,
		repository:      repository,
		runProcessors:   make([]RunProcessor, 0),
		applyProcessors: make([]ApplyProcessor, 0),
	}
}

func (s *Controller) WithProcessor(p RunApplyProcessor) *Controller {
	return s.WithRunProcessor(p).WithApplyProcessor(p)
}

func (s *Controller) WithRunProcessor(p RunProcessor) *Controller {
	s.runProcessors = append(s.runProcessors, p)
	return s
}

func (s *Controller) WithApplyProcessor(p ApplyProcessor) *Controller {
	s.applyProcessors = append(s.applyProcessors, p)
	return s
}

func (s *Controller) Name() string {
	return IoTScenarioIntent
}

func (s *Controller) IsRunnable(_ context.Context, request *scenarios.TScenarioRunRequest) bool {
	for _, p := range s.runProcessors {
		if p.IsRunnable(request) {
			return true
		}
	}
	return false
}

func (s *Controller) IsApplicable(ctx context.Context, request *scenarios.TScenarioApplyRequest) bool {
	var aaProto protos.TApplyArguments // apply request always has not nil apply argument; otherwise it's not apply request
	if err := request.Arguments.UnmarshalTo(&aaProto); err != nil {
		ctxlog.Debugf(ctx, s.logger, "cannot unmarshal apply_arguments: %s", err)
		return false
	}
	for _, p := range s.applyProcessors {
		if p.IsApplicable(request, &aaProto) {
			return true
		}
	}
	return false
}

func (s *Controller) IsContinuable(ctx context.Context, request *scenarios.TScenarioApplyRequest) bool {
	// FIXME: when we would have any continuable intent
	return false
}

func (s *Controller) Run(ctx context.Context, request *scenarios.TScenarioRunRequest, user model.User) (*scenarios.TScenarioRunResponse, error) {
	clientInfo := libmegamind.NewClientInfo(request.BaseRequest.ClientInfo)

	// https://st.yandex-team.ru/IOT-1601
	if clientInfo.IsSelfDrivingCar() {
		message := "requests from self driving cars are not supported, exiting megamind controller"
		ctxlog.Info(ctx, s.logger, message)
		setrace.InfoLogEvent(ctx, s.logger, message)
		return NlgRunResponse(ctx, s.logger, nlg.SelfDrivingCarsNotSupported)
	}

	var processor RunProcessor
	for _, p := range s.runProcessors {
		if p.IsRunnable(request) {
			message := fmt.Sprintf("`%s` processor has been chosen", p.Name())
			ctxlog.Info(ctx, s.logger, message, log.Any("scenario", p.Name()))
			setrace.InfoLogEvent(ctx, s.logger, message, log.Any("scenario", p.Name()))
			processor = p
			break
		}
	}

	if processor == nil {
		return Irrelevant(ctx, s.logger, ProcessorNotFound)
	}

	if user.IsEmpty() {
		if clientInfo.IsElariWatch() {
			return NlgRunResponse(ctx, s.logger, nlg.UnsupportedClient)
		}
		if !clientInfo.IsWindows() && clientInfo.IsYaMessenger() { // https://st.yandex-team.ru/ALICE-7178
			return NlgRunResponse(ctx, s.logger, nlg.UnsupportedClient)
		}
		return NlgRunResponse(ctx, s.logger, nlg.NeedLogin)
	}

	if patchedUser, isBusinessUser := s.checkBusinessUser(ctx, request.GetBaseRequest(), user, clientInfo); isBusinessUser {
		ctx = alice4business.WithA4BGuestUserID(ctx, user.ID)
		ctx = userctx.WithUser(ctx, patchedUser.ToUserctxUser())
		user = patchedUser
	}

	// todo: replace else part of condition with error after datasource becomes required
	var userDevicesAndScenarios model.ExtendedUserInfo
	iotDataSource, iotDataSourceOk := request.DataSources[int32(pcommon.EDataSourceType_IOT_USER_INFO)]
	if iotDataSourceOk {
		iotUserInfo := iotDataSource.GetIoTUserInfo()
		if iotUserInfo == nil {
			datasourceErr := xerrors.New("datasource is marked as IOT_USER_INFO but does not hold data")
			return ErrorRunResponse(ctx, s.logger, datasourceErr)
		}
		var userInfo model.UserInfo
		if err := userInfo.FromUserInfoProto(ctx, iotUserInfo); err != nil {
			conversionErr := xerrors.Errorf("unable to convert data from datasource to UserInfo: %w", err)
			return ErrorRunResponse(ctx, s.logger, conversionErr)
		}
		userDevicesAndScenarios = model.ExtendedUserInfo{
			User:     user,
			UserInfo: userInfo,
		}
	} else {
		userInfo, userInfoErr := s.repository.UserInfo(ctx, user)
		if userInfoErr != nil {
			switch {
			case xerrors.Is(userInfoErr, &model.UnknownUserError{}):
				return NlgRunResponse(ctx, s.logger, nlg.NeedDevices)
			default:
				ctxlog.Warnf(ctx, s.logger, "error getting UserInfo from db: %v", userInfoErr)
				setrace.ErrorLogEvent(
					ctx, s.logger,
					fmt.Sprintf("error getting UserInfo from db: %v", userInfoErr),
				)
			}
			return ErrorRunResponse(ctx, s.logger, userInfoErr)
		}
		userDevicesAndScenarios = model.ExtendedUserInfo{
			User:     user,
			UserInfo: userInfo,
		}
	}

	//// if you need to dump run request data to use it in logs later, uncomment this part
	//requestID := request.GetBaseRequest().GetRequestId()
	//dumpName := fmt.Sprintf("%s.protobuf", requestID)
	//if err := protodumper.DumpToFile("/logs/testdata", dumpName, request); err != nil {
	//	ctxlog.Warnf(ctx, s.logger, "can't dump %s: %v", dumpName, err)
	//}

	return processor.Run(ctx, request, userDevicesAndScenarios)
}

func (s *Controller) Apply(ctx context.Context, request *scenarios.TScenarioApplyRequest, user model.User) (*scenarios.TScenarioApplyResponse, error) {
	var aaProto protos.TApplyArguments // apply request always has not nil apply argument; otherwise it's not apply request
	if err := request.Arguments.UnmarshalTo(&aaProto); err != nil {
		ctxlog.Warnf(ctx, s.logger, "cannot unmarshal apply_arguments: %s", err)
		setrace.ErrorLogEvent(ctx, s.logger, fmt.Sprintf("cannot unmarshal apply_arguments: %s", err))
		return ErrorApplyResponse(ctx, s.logger, err)
	}

	var processor ApplyProcessor
	for _, p := range s.applyProcessors {
		if p.IsApplicable(request, &aaProto) {
			processor = p
			break
		}
	}

	if processor == nil {
		msg := "cannot find processor for request"
		ctxlog.Warn(ctx, s.logger, msg)
		setrace.ErrorLogEvent(ctx, s.logger, msg)
		return ErrorApplyResponse(ctx, s.logger, xerrors.New(msg))
	}

	clientInfo := libmegamind.NewClientInfo(request.BaseRequest.ClientInfo)
	if patchedUser, isBusinessUser := s.checkBusinessUser(ctx, request.GetBaseRequest(), user, clientInfo); isBusinessUser {
		ctx = alice4business.WithA4BGuestUserID(ctx, user.ID)
		ctx = userctx.WithUser(ctx, patchedUser.ToUserctxUser())
		user = patchedUser
	}

	return processor.Apply(ctx, request, &aaProto, user)
}

func (s *Controller) Continue(ctx context.Context, request *scenarios.TScenarioApplyRequest, user model.User) (*scenarios.TScenarioContinueResponse, error) {
	// FIXME: when we would have any continuable intent
	return nil, xerrors.New("failed to continue through bulbasaur processor: no continuable processors")
}

func (s *Controller) checkBusinessUser(ctx context.Context, baseRequest *scenarios.TScenarioBaseRequest, user model.User, clientInfo libmegamind.ClientInfo) (model.User, bool) {
	// resolve station user if alice4business config is given
	// notify @pazus or @kuptservol when changing this condition
	alice4BusinessConfig := baseRequest.GetOptions().GetQuasarAuxiliaryConfig().GetAlice4Business()
	if stationOwnerID := alice4BusinessConfig.GetSmartHomeUid(); stationOwnerID != 0 {
		ctxlog.Info(ctx, s.logger, fmt.Sprintf("Alice4Business station detected %s, owner replaced to %d", clientInfo.DeviceID, stationOwnerID))
		setrace.InfoLogEvent(
			ctx, s.logger,
			fmt.Sprintf("Alice4Business station detected %s, owner replaced to %d", clientInfo.DeviceID, stationOwnerID),
		)
		user.ID = stationOwnerID
		return user, true
	}
	return user, false
}

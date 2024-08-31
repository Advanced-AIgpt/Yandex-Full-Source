package megamind

import (
	"context"
	"fmt"
	"runtime/debug"
	"sort"
	"strings"
	"sync"
	"time"

	"github.com/golang/protobuf/ptypes"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/action"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/experiments"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/query"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/scenario"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/settings"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/megamind"
	"a.yandex-team.ru/alice/iot/bulbasaur/logging"
	"a.yandex-team.ru/alice/iot/bulbasaur/logging/doublelog"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/nlg"
	"a.yandex-team.ru/alice/iot/bulbasaur/protos"
	"a.yandex-team.ru/alice/library/go/contexter"
	"a.yandex-team.ru/alice/library/go/inflector"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	libnlg "a.yandex-team.ru/alice/library/go/nlg"
	"a.yandex-team.ru/alice/library/go/setrace"
	"a.yandex-team.ru/alice/library/go/timestamp"
	megamindcommonpb "a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type BegemotProcessor struct {
	Logger             log.Logger
	Inflector          inflector.IInflector
	QueryController    query.IController
	ActionController   action.IController
	ScenarioController *scenario.Controller
	SettingsController settings.IController
}

func (b *BegemotProcessor) Name() string {
	return IoTScenarioIntent
}

func (b *BegemotProcessor) IsRunnable(request *scenarios.TScenarioRunRequest) bool {
	_, begemotDataSourceOk := request.DataSources[int32(megamindcommonpb.EDataSourceType_BEGEMOT_IOT_NLU_RESULT)]
	return begemotDataSourceOk
}

func (b *BegemotProcessor) Run(ctx context.Context, request *scenarios.TScenarioRunRequest, u model.ExtendedUserInfo) (*scenarios.TScenarioRunResponse, error) {
	megamindExperiments := make(map[string]bool, 0)
	for k := range request.GetBaseRequest().GetExperiments().GetFields() {
		megamindExperiments[k] = true
	}

	if granetProcessorShouldBeUsed(ctx, megamindExperiments) {
		return Irrelevant(ctx, b.Logger, GranetProcessorIsPreferred)
	}

	begemotDataSource, begemotDataSourceOk := request.DataSources[int32(megamindcommonpb.EDataSourceType_BEGEMOT_IOT_NLU_RESULT)]
	if !begemotDataSourceOk {
		return nil, xerrors.New("No begemot data source found")
	}
	if noSmartHomeDevices(u.UserInfo) {
		return NlgRunResponse(ctx, b.Logger, nlg.NeedDevices)
	}
	tandemDataSource := request.DataSources[int32(megamindcommonpb.EDataSourceType_TANDEM_ENVIRONMENT_STATE)]

	return b.processBegemotIOTNluResult(ctx, u, begemotRequestData{
		clientInfo:              libmegamind.NewClientInfo(request.BaseRequest.ClientInfo),
		protoBegemotDataSource:  begemotDataSource,
		protoTandemDataSource:   tandemDataSource,
		sources:                 nil,
		megamindExperimentFlags: megamindExperiments,
	})
}

type begemotRequestData struct {
	clientInfo              libmegamind.ClientInfo
	protoBegemotDataSource  *scenarios.TDataSource
	protoTandemDataSource   *scenarios.TDataSource
	sources                 *AdditionalSources
	megamindExperimentFlags map[string]bool
}

func (r begemotRequestData) isTandem() bool {
	tandemState := r.protoTandemDataSource.GetTandemEnvironmentState()
	if tandemState == nil {
		return false
	}
	clientDeviceID := r.clientInfo.DeviceID
	for _, tandemGroup := range tandemState.GetGroups() {
		for _, tandemDevice := range tandemGroup.GetDevices() {
			if tandemDevice.GetId() == clientDeviceID {
				return true
			}
		}
	}
	return false
}

func (b *BegemotProcessor) processBegemotIOTNluResult(ctx context.Context, u model.ExtendedUserInfo, begemotRequestData begemotRequestData) (*scenarios.TScenarioRunResponse, error) {
	var nluResult *scenarios.TBegemotIotNluResult
	if nluResult = begemotRequestData.protoBegemotDataSource.GetBegemotIotNluResult(); nluResult == nil {
		return Irrelevant(ctx, b.Logger, MissingIotNluResult)
	}

	ctxlog.Info(ctx, b.Logger, "raw_hypotheses", logging.ProtoJSON("data", nluResult))
	setrace.InfoLogEvent(
		ctx, b.Logger, "raw_hypotheses",
		logging.ProtoJSON("data", nluResult),
	)

	ctxlog.Info(ctx, b.Logger, "user_info", log.Any("data", u.UserInfo))
	setrace.InfoLogEvent(
		ctx, b.Logger, "user_info",
		log.Any("data", u.UserInfo),
	)

	hs, err := megamind.NewHypotheses(nluResult.GetTypedHypotheses())
	if err != nil {
		ctxlog.Warnf(ctx, b.Logger, "error parsing BegemotIotNluResult.TypedHypotheses: %v", err)
		setrace.ErrorLogEvent(
			ctx, b.Logger,
			fmt.Sprintf("error parsing BegemotIotNluResult.Hypotheses: %v", err),
		)
		return ErrorRunResponse(ctx, b.Logger, err)
	}
	// FIXME: do not use dto.megamind.Hypotheses in ApplyArguments, use model.Hypotheses instead

	if nluResult.HypothesesType == scenarios.TBegemotIotNluResult_DEMO_BASED {
		isTandem := begemotRequestData.isTandem()
		if isTandem {
			ctxlog.Info(ctx, b.Logger, "tandem environment state",
				logging.ProtoJSON("data", begemotRequestData.protoTandemDataSource.GetTandemEnvironmentState()),
				log.Bool("is_tandem", isTandem),
			)
			setrace.InfoLogEvent(
				ctx, b.Logger, "tandem environment state",
				logging.ProtoJSON("data", begemotRequestData.protoTandemDataSource.GetTandemEnvironmentState()),
				log.Bool("is_tandem", isTandem),
			)
		}
		return b.processDemoHypotheses(ctx, hs, isTandem)
	}

	if sources := begemotRequestData.sources; sources != nil {
		if additionalTime, ok := sources.Time(); ok {
			if additionalTime.IsRelative() {
				msg := "Cannot do: user specified relative additional time"
				ctxlog.Info(ctx, b.Logger, msg)
				setrace.InfoLogEvent(ctx, b.Logger, msg)
				return NlgRunResponse(ctx, b.Logger, nlg.CannotDo)
			}
			hs.AppendTime(additionalTime) // we have additional time specified, let's use it for our hypotheses
		}
	}

	userLocalTime := time.Now().In(begemotRequestData.clientInfo.GetLocation(model.DefaultTimezone)) // should be in user local timezone in case of abs time in hypothesis
	hypotheses := hs.ToHypotheses(userLocalTime)
	hypothesesTypeMap := hypotheses.GroupByType()
	hypothesesTypesCount := len(hypothesesTypeMap)
	if hypothesesTypesCount != 1 {
		msg := fmt.Sprintf("invalid number of hypotheses types: %d", hypothesesTypesCount)
		ctxlog.Info(ctx, b.Logger, msg, log.Any("hypotheses_types", hypothesesTypeMap))
		setrace.InfoLogEvent(
			ctx, b.Logger,
			msg,
			log.Any("hypotheses_types", hypothesesTypeMap),
		)
		return NlgRunResponse(ctx, b.Logger, nlg.CannotDo)
	}
	for hypothesesType, hypotheses := range hypothesesTypeMap {
		switch hypothesesType {
		case model.ActionHypothesisType:
			return b.processActionHypotheses(ctx, begemotRequestData, hs, hypotheses, u)
		case model.QueryHypothesisType:
			return b.processQueryHypotheses(ctx, begemotRequestData, hs, hypotheses, u)
		default:
			msg := fmt.Sprintf("unknown hypothesis type: %s", hypothesesType)
			ctxlog.Info(ctx, b.Logger, msg, log.Any("hypotheses_types", hypothesesTypeMap))
			setrace.InfoLogEvent(
				ctx, b.Logger,
				msg,
				log.Any("hypotheses_types", hypothesesTypeMap),
			)
		}
	}
	return NlgRunResponse(ctx, b.Logger, nlg.CannotDo)
}

func (b *BegemotProcessor) processQueryHypotheses(ctx context.Context, begemotDataSources begemotRequestData, hs megamind.Hypotheses, hypotheses model.Hypotheses, u model.ExtendedUserInfo) (*scenarios.TScenarioRunResponse, error) {
	filtrationResults, extractedQueries := model.ExtractQueries(hypotheses, u.UserInfo, begemotDataSources.isTandem(), begemotDataSources.clientInfo.IsIotApp())
	extractedQueriesNum := len(extractedQueries)
	msg := fmt.Sprintf("%d queries found", extractedQueriesNum)
	ctxlog.Info(ctx, b.Logger, msg, log.Any("extracted_queries", extractedQueries))
	setrace.InfoLogEvent(
		ctx, b.Logger,
		msg,
		log.Any("extracted_queries", extractedQueries),
	)
	filtrationReasons := make([]model.HypothesisFilterReason, 0, len(filtrationResults))
	for _, result := range filtrationResults {
		filtrationReasons = append(filtrationReasons, result.Reason)
	}
	if len(filtrationResults) > 0 {
		ctxlog.Infof(ctx, b.Logger, "%d hypotheses filtered out", len(filtrationResults))
		setrace.InfoLogEvent(
			ctx, b.Logger,
			fmt.Sprintf("%d hypotheses filtered out", len(filtrationResults)),
			log.Any("hypotheses_filtration_reasons", filtrationReasons),
		)
	}

	extractedQueriesAfterConflicts := model.ResolveQueriesConflicts(extractedQueries)
	extractedQueriesNumAfterConflicts := len(extractedQueriesAfterConflicts)
	if extractedQueriesNum != extractedQueriesNumAfterConflicts {
		extractedQueries = extractedQueriesAfterConflicts
		extractedQueriesNum = extractedQueriesNumAfterConflicts
		msg := fmt.Sprintf("%d queries left after conflict resolution", extractedQueriesNum)
		ctxlog.Info(ctx, b.Logger, msg, log.Any("extracted_queries", extractedQueries))
		setrace.InfoLogEvent(
			ctx, b.Logger,
			msg,
			log.Any("extracted_queries", extractedQueries),
		)
	}

	// we have to filter out query hypotheses
	// for now we support only 1 hypothesis
	switch {
	case extractedQueriesNum == 0:
		if len(filtrationResults) == 0 {
			return NlgRunResponse(ctx, b.Logger, nlg.CannotDo)
		}
		switch filtrationResults[0].Reason {
		case model.TandemTVHypothesisFilterReason:
			return Irrelevant(ctx, b.Logger, TandemTVHypotheses)
		case model.InappropriateHouseholdFilterReason:
			return NlgRunResponse(ctx, b.Logger, nlg.InappropriateHousehold)
		case model.InappropriateRoomFilterReason:
			return NlgRunResponse(ctx, b.Logger, nlg.InappropriateRoom)
		case model.InappropriateGroupFilterReason:
			return NlgRunResponse(ctx, b.Logger, nlg.InappropriateGroup)
		case model.InappropriateCapabilityFilterReason:
			return NlgRunResponse(ctx, b.Logger, nlg.InvalidAction)
		case model.ShouldSpecifyHouseholdFilterReason:
			ctxlog.Infof(ctx, b.Logger, "asking to specify the household by frame action")
			return b.nlgWithHouseholdSpecifyRequest(ctx, begemotDataSources.protoBegemotDataSource, hypotheses.AnyHypothesisType(), u)
		case model.InappropriateDevicesFilterReason:
			fallthrough
		default:
			return NlgRunResponse(ctx, b.Logger, nlg.CannotDo)
		}
	case extractedQueriesNum > 1:
		return NlgRunResponse(ctx, b.Logger, nlg.QueryCannotDo)
	}

	extractedQuery := extractedQueries[0]
	selectedHypotheses := megamind.SelectedHypotheses{}
	selectedHypotheses.PopulateFromQuery(extractedQuery)
	aa := &megamind.DevicesQueryApplyArguments{
		Hypotheses:         hs,
		SelectedHypotheses: selectedHypotheses,
		ExtractedQuery:     extractedQuery,
	}

	applyArguments := &protos.TApplyArguments{
		Value: &protos.TApplyArguments_DevicesQueryApplyArguments{
			DevicesQueryApplyArguments: aa.ToProto(),
		},
	}
	return BuildRunResponse(ctx, b.Logger, applyArguments)
}

func (b *BegemotProcessor) processActionHypotheses(ctx context.Context, begemotDataSources begemotRequestData, hs megamind.Hypotheses, hypotheses model.Hypotheses, u model.ExtendedUserInfo) (*scenarios.TScenarioRunResponse, error) {
	filtrationResults, extractedActions, filteredHypotheses := model.ExtractActions(
		b.Inflector,
		hypotheses,
		u.UserInfo,
		begemotDataSources.isTandem(),
		begemotDataSources.clientInfo.IsIotApp(),
	)

	extractedActionsNum := len(extractedActions)
	ctxlog.Infof(ctx, b.Logger, "%d valid hypotheses found", extractedActionsNum)
	setrace.InfoLogEvent(
		ctx, b.Logger,
		fmt.Sprintf("%d valid hypotheses found", extractedActionsNum),
		log.Any("extracted_actions", extractedActions),
	)
	filtrationReasons := make([]model.HypothesisFilterReason, 0, len(filtrationResults))
	for _, result := range filtrationResults {
		filtrationReasons = append(filtrationReasons, result.Reason)
	}
	if len(filtrationResults) > 0 {
		ctxlog.Infof(ctx, b.Logger, "%d hypotheses filtered out", len(filtrationResults))
		setrace.InfoLogEvent(
			ctx, b.Logger,
			fmt.Sprintf("%d hypotheses filtered out", len(filtrationResults)),
			log.Any("hypotheses_filtration_reasons", filtrationReasons),
		)
	}
	if extractedActionsNum == 0 {
		if len(filtrationResults) == 0 {
			return NlgRunResponse(ctx, b.Logger, nlg.CannotDo)
		}
		switch filtrationResults[0].Reason {
		case model.TandemTVHypothesisFilterReason:
			return Irrelevant(ctx, b.Logger, TandemTVHypotheses)
		case model.InappropriateHouseholdFilterReason:
			return NlgRunResponse(ctx, b.Logger, nlg.InappropriateHousehold)
		case model.InappropriateRoomFilterReason:
			return NlgRunResponse(ctx, b.Logger, nlg.InappropriateRoom)
		case model.InappropriateGroupFilterReason:
			return NlgRunResponse(ctx, b.Logger, nlg.InappropriateGroup)
		case model.InappropriateCapabilityFilterReason:
			return NlgRunResponse(ctx, b.Logger, nlg.InvalidAction)
		case model.InappropriateTurnOnAllDevicesFilterReason: // https://st.yandex-team.ru/IOT-801
			return NlgRunResponse(ctx, b.Logger, nlg.TurnOnDevicesIsForbidden)
		case model.ShouldSpecifyHouseholdFilterReason:
			ctxlog.Infof(ctx, b.Logger, "asking to specify the household by frame action")
			return b.nlgWithHouseholdSpecifyRequest(
				ctx,
				begemotDataSources.protoBegemotDataSource,
				hypotheses.AnyHypothesisType(),
				u,
			)
		case model.InappropriateDevicesFilterReason:
			fallthrough
		default:
			return NlgRunResponse(ctx, b.Logger, nlg.CannotDo)
		}
	}

	delayedActionsNum := extractedActions.CountDelayedActions()
	isSameHypothesisAction := filteredHypotheses.HaveSameType() && filteredHypotheses.HaveSameValue()
	isNotSupportedForDelayedAction := !isSameHypothesisAction || !extractedActions.HaveSameTimeInfo() || extractedActions.HaveScenarioActions()

	if delayedActionsNum > 0 && delayedActionsNum != extractedActionsNum {
		ctxlog.Infof(ctx, b.Logger, "Cannot do: received mixed delayed and common actions (num delayed: %d, total: %d)", delayedActionsNum, extractedActionsNum)
		setrace.InfoLogEvent(
			ctx, b.Logger,
			fmt.Sprintf("Cannot do: received mixed delayed and common actions (num delayed: %d, total: %d)", delayedActionsNum, extractedActionsNum),
		)
		return NlgRunResponse(ctx, b.Logger, nlg.CannotDo)
	} else if delayedActionsNum > 0 && isNotSupportedForDelayedAction {
		ctxlog.Infof(ctx, b.Logger,
			"Cannot do: received not supported delayed actions (same action: %t, same time info: %t, includes scenario: %t)",
			isSameHypothesisAction, extractedActions.HaveSameTimeInfo(), extractedActions.HaveScenarioActions())
		setrace.InfoLogEvent(
			ctx, b.Logger,
			fmt.Sprintf("Cannot do: received not supported delayed actions (same action: %t, same time info: %t, includes scenario: %t)",
				isSameHypothesisAction, extractedActions.HaveSameTimeInfo(), extractedActions.HaveScenarioActions()),
		)

		if !isSameHypothesisAction {
			for _, h := range filteredHypotheses {
				ctxlog.Infof(ctx, b.Logger, "Hypothesis %d: type=%s, value=%+v", h.ID, h.Type, h.Value)
				setrace.InfoLogEvent(ctx, b.Logger, fmt.Sprintf("Hypothesis %d: type=%s, value=%+v", h.ID, h.Type, h.Value))
			}
		}

		return NlgRunResponse(ctx, b.Logger, nlg.CannotDo)
	} else if delayedActionsNum > 0 {
		// we have only delayed hypotheses with equal time info, let's validate it
		timeInfo := extractedActions[0].TimeInfo
		createdTime := extractedActions[0].CreatedTime

		if timeInfo.IsInterval && !extractedActions.IsSupportedForIntervalActions() {
			ctxlog.Info(ctx, b.Logger, "Cannot do: interval action is unsupported by capability (only on off is supported)")
			setrace.InfoLogEvent(ctx, b.Logger, "Cannot do: interval action is unsupported by capability (only on off is supported)")
			return NlgRunResponse(ctx, b.Logger, nlg.CannotDo)
		}

		if response, err := timeInfo.Validate(createdTime); err != nil {
			ctxlog.Infof(ctx, b.Logger, "Cannot do: %v", err)
			setrace.InfoLogEvent(ctx, b.Logger, fmt.Sprintf("Cannot do: %v", err))
			return NlgRunResponse(ctx, b.Logger, response)
		}

		if !timeInfo.IsInterval && !timeInfo.DateTime.IsTimeSpecified {
			hasTimeSource := false
			if sources := begemotDataSources.sources; sources != nil {
				_, hasTimeSource = sources.Time()
			}

			if !hasTimeSource {
				// user specified delayed action without time, we should ask user about time
				return b.nlgWithTimeSpecifyRequest(ctx, begemotDataSources.protoBegemotDataSource)
			}
		}
	}

	selectedHypotheses := megamind.SelectedHypotheses{}
	selectedHypotheses.PopulateFromActions(extractedActions)

	aa := &megamind.DeviceActionsApplyArguments{
		Hypotheses:         hs,
		SelectedHypotheses: selectedHypotheses,
		ExtractedActions:   extractedActions,
		Devices:            u.UserInfo.Devices,
	}
	applyArguments := &protos.TApplyArguments{
		Value: &protos.TApplyArguments_DeviceActionsApplyArguments{
			DeviceActionsApplyArguments: aa.ToProto(),
		},
	}
	return BuildRunResponse(ctx, b.Logger, applyArguments)
}

func (b *BegemotProcessor) processDemoHypotheses(ctx context.Context, hs megamind.Hypotheses, isTandem bool) (*scenarios.TScenarioRunResponse, error) {
	if len(hs) == 0 {
		return Irrelevant(ctx, b.Logger, MissingHypotheses)
	}
	setrace.InfoLogEvent(ctx, b.Logger, "processing demo hypotheses")
	ctxlog.Info(ctx, b.Logger, "processing demo hypotheses")
	hypothesis := hs[0] // https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/library/scenarios/iot/iot.cpp?rev=6744980#L275
	typeToDemoEntities := make(map[string][]megamind.RawEntity)
	for _, e := range hypothesis.RawEntities {
		if e.IsDemoEntity() {
			typeToDemoEntities[e.Type] = append(typeToDemoEntities[e.Type], e)
		}
	}

	if len(typeToDemoEntities) == 1 {
		var entityType string
		var demoEntities []megamind.RawEntity
		for k, v := range typeToDemoEntities {
			entityType = k
			demoEntities = v
		}
		firstEntity := demoEntities[0]
		value := firstEntity.Value

		// value looks like "[\"demo--other24\"]" or "[\"demo--nursery\"]"
		extractedValue, _ := value.(string)
		extractedValue = strings.TrimPrefix(strings.Trim(extractedValue, "\"[]"), "demo--") // "other24" / "nursery"
		key := DemoEntityValueToName[extractedValue]

		setrace.InfoLogEvent(ctx, b.Logger,
			"demo_entity",
			log.Any("entity_type", entityType), log.Any("entity", firstEntity),
		)
		ctxlog.Info(ctx, b.Logger, "demo_entity", log.Any("entity", map[string]interface{}{
			"entity_type":  entityType,
			"first_entity": firstEntity}))
		if len(key) == 0 {
			return NlgRunResponse(ctx, b.Logger, nlg.CheckSettings)
		} else if entityType == "device" && len(demoEntities) == 1 {
			if hypothesis.Type == model.QueryHypothesisType {
				return NlgRunResponse(ctx, b.Logger, nlg.CannotFindDevice(key))
			}
			switch key {
			case "вентилятор":
				return NlgRunResponse(ctx, b.Logger, nlg.CannotFindFan)
			case "обогреватель":
				return NlgRunResponse(ctx, b.Logger, nlg.CannotFindHeater)
			case "кондиционер":
				return NlgRunResponse(ctx, b.Logger, nlg.CannotFindAC)
			case "ресивер":
				return NlgRunResponse(ctx, b.Logger, nlg.CannotFindTVBox)
			case "телевизор":
				if isTandem {
					return Irrelevant(ctx, b.Logger, TandemTVHypotheses)
				}
				return NlgRunResponse(ctx, b.Logger, nlg.CannotFindTV)
			case "электрокамин":
				return NlgRunResponse(ctx, b.Logger, nlg.CannotFindFireplace)
			default:
				return NlgRunResponse(ctx, b.Logger, nlg.CannotFindDevice(key))
			}
		} else if entityType == "device" && len(demoEntities) > 1 {
			return NlgRunResponse(ctx, b.Logger, nlg.CannotFindDevices)
		} else if entityType == "room" && len(demoEntities) == 1 && key == "комната" {
			return NlgRunResponse(ctx, b.Logger, nlg.CannotFindRoom)
		} else if entityType == "room" && len(demoEntities) == 1 {
			return NlgRunResponse(ctx, b.Logger, nlg.CannotFindRequestedRoom(key))
		} else if entityType == "room" && len(demoEntities) > 1 {
			return NlgRunResponse(ctx, b.Logger, nlg.CannotFindRooms)
		} else if entityType == "device_type" && len(demoEntities) == 1 && extractedValue == string(model.LightDeviceType) {
			return NlgRunResponse(ctx, b.Logger, nlg.CannotFindLightDevices)
		}
	}
	return NlgRunResponse(ctx, b.Logger, nlg.CheckSettings)
}

func (b *BegemotProcessor) nlgWithTimeSpecifyRequest(ctx context.Context, dataSource *scenarios.TDataSource) (*scenarios.TScenarioRunResponse, error) {
	begemotProto, err := ptypes.MarshalAny(dataSource)
	if err != nil {
		return ErrorRunResponse(ctx, b.Logger, err)
	}
	return NlgRunResponseBuilder(ctx, b.Logger, nlg.NoTimeSpecified).
		WithCallbackFrameAction(GetTimeSpecifyEmptyCallback()).
		WithState(begemotProto).
		ExpectRequest().
		Build(), nil
}

func (b *BegemotProcessor) nlgWithHouseholdSpecifyRequest(ctx context.Context, dataSource *scenarios.TDataSource, hypothesisType model.HypothesisType, u model.ExtendedUserInfo) (*scenarios.TScenarioRunResponse, error) {
	begemotProto, err := ptypes.MarshalAny(dataSource)
	if err != nil {
		return ErrorRunResponse(ctx, b.Logger, err)
	}
	var noHouseholdSpecifiedNLG libnlg.NLG
	if hypothesisType == model.QueryHypothesisType {
		noHouseholdSpecifiedNLG = nlg.NoHouseholdSpecifiedQuery
	} else {
		noHouseholdSpecifiedNLG = nlg.NoHouseholdSpecifiedAction
	}
	runResponseBuilder := NlgRunResponseBuilder(ctx, b.Logger, noHouseholdSpecifiedNLG).
		WithState(begemotProto).
		ExpectRequest()
	householdsCallbacks := GetHouseholdSpecifyCallbacks(u.UserInfo.Households, b.Inflector)
	for _, callbackAction := range householdsCallbacks {
		runResponseBuilder.WithCallbackFrameAction(callbackAction)
	}

	return runResponseBuilder.Build(), nil
}

func (b *BegemotProcessor) IsApplicable(_ *scenarios.TScenarioApplyRequest, arguments *protos.TApplyArguments) bool {
	return arguments.GetDeviceActionsApplyArguments() != nil || arguments.GetDevicesQueryApplyArguments() != nil
}

func (b *BegemotProcessor) Apply(ctx context.Context, request *scenarios.TScenarioApplyRequest, arguments *protos.TApplyArguments, user model.User) (*scenarios.TScenarioApplyResponse, error) {
	origin := newOrigin(ctx, libmegamind.NewClientInfo(request.GetBaseRequest().GetClientInfo()), user)
	doublelog.Info(
		ctx, b.Logger,
		"Apply stage",
		log.Any("scenario", b.Name()),
		log.Any("origin", origin),
	)
	if aaProto := arguments.GetDeviceActionsApplyArguments(); aaProto != nil {
		return b.processActionApplyArguments(ctx, request, aaProto, origin)
	} else if aaProto := arguments.GetDevicesQueryApplyArguments(); aaProto != nil {
		return b.processQueryApplyArguments(ctx, request, aaProto, origin)
	} else {
		return nil, xerrors.New("unknown apply arguments type")
	}
}

func (b *BegemotProcessor) processQueryApplyArguments(ctx context.Context, request *scenarios.TScenarioApplyRequest, aaProto *protos.DevicesQueryApplyArguments, origin model.Origin) (*scenarios.TScenarioApplyResponse, error) {
	var applyArguments megamind.DevicesQueryApplyArguments
	applyArguments.FromProto(aaProto)

	analyticsInfo := formAnalyticsInfo(aaProto.Hypotheses, aaProto.SelectedHypotheses)
	type queryResultMsg struct {
		result QueryResult
		err    error
	}
	resultCh := make(chan queryResultMsg, 1)
	noCancelCtx := contexter.NoCancel(ctx)
	go func() {
		defer func() {
			if r := recover(); r != nil {
				err := xerrors.Errorf("caught panic in apply: %v", r)
				ctxlog.Warn(ctx, b.Logger, fmt.Sprintf("%v", err), log.Any("stacktrace", string(debug.Stack())))
				resultCh <- queryResultMsg{err: err}
			}
		}()
		devices, deviceStates, err := b.QueryController.UpdateDevicesState(ctx, applyArguments.ExtractedQuery.Devices, origin)
		resultCh <- queryResultMsg{result: QueryResult{Devices: devices, DeviceStates: deviceStates}, err: err}
	}()

	timeout, err := timestamp.ComputeTimeout(ctx, MMApplyTimeout)
	if err != nil {
		ctxlog.Warnf(ctx, b.Logger, "unable to get timestamper from context, falling back to default apply-timeout: %dms",
			DefaultApplyTimeout.Milliseconds())
		timeout = DefaultApplyTimeout
	}

	select {
	case resultMsg := <-resultCh:
		if err := resultMsg.err; err != nil {
			ctxlog.Warnf(ctx, b.Logger, "error in querying devices: %v", err)
			setrace.ErrorLogEvent(
				ctx, b.Logger,
				fmt.Sprintf("error in querying devices: %v", err),
			)
		}
		return NlgApplyResponse(noCancelCtx, b.Logger, b.processQueryResult(ctx, request, origin, applyArguments.ExtractedQuery, resultMsg.result), analyticsInfo)
	case <-time.After(timeout):
		setrace.InfoLogEvent(
			ctx, b.Logger,
			fmt.Sprintf("Timeout was reached after %dms, answering with default timeout NLG", timeout.Milliseconds()),
		)
		outputResponse := libmegamind.OutputResponse{NLG: nlg.QueryTimeout}
		return NlgApplyResponse(noCancelCtx, b.Logger, outputResponse, analyticsInfo)
	}
}

func (b *BegemotProcessor) processQueryResult(ctx context.Context, request *scenarios.TScenarioApplyRequest, origin model.Origin, extractedQuery model.ExtractedQuery, result QueryResult) libmegamind.OutputResponse {
	devices, deviceStates := result.Devices, result.DeviceStates

	onlineDevices := make(model.Devices, 0, len(devices))
	offlineDevices := make(model.Devices, 0, len(devices))
	notFoundDevices := make(model.Devices, 0, len(devices))
	targetNotFoundDevices := make(model.Devices, 0, len(devices))
	otherErrDevices := make(model.Devices, 0, len(devices))

	deviceCapabilityResults := make(DeviceCapabilityQueryResults, 0, len(devices))
	devicePropertyResults := make(DevicePropertyQueryResults, 0, len(devices))
	for _, device := range devices {
		switch deviceState := deviceStates[device.ID]; deviceState {
		case model.OnlineDeviceStatus:
			onlineDevices = append(onlineDevices, device)

			switch searchFor := extractedQuery.SearchFor; searchFor.Target {
			case model.CapabilityTarget:
				c, ok := device.GetCapabilityByTypeAndInstance(model.CapabilityType(searchFor.Type), searchFor.Instance)
				if !ok {
					targetNotFoundDevices = append(targetNotFoundDevices, device)
					continue
				}
				deviceCapabilityResults = append(deviceCapabilityResults, DeviceCapabilityQueryResult{
					ID: device.ID, Name: device.Name, DeviceType: device.Type, Room: device.Room, Capability: c,
				})
			case model.PropertyTarget:
				p, ok := device.GetPropertyByTypeAndInstance(model.PropertyType(searchFor.Type), searchFor.Instance)
				if !ok {
					targetNotFoundDevices = append(targetNotFoundDevices, device)
					continue
				}
				devicePropertyResults = append(devicePropertyResults, DevicePropertyQueryResult{
					ID: device.ID, Name: device.Name, DeviceType: device.Type, Room: device.Room, Property: p,
				})
			case model.ModeTarget, model.StateTarget:
				// skip, we will use onlineDevices slice to form answer
			default:
				otherErrDevices = append(otherErrDevices, device)
			}
		case model.OfflineDeviceStatus:
			offlineDevices = append(offlineDevices, device)
		case model.NotFoundDeviceStatus:
			notFoundDevices = append(notFoundDevices, device)
		default:
			otherErrDevices = append(otherErrDevices, device)
		}
	}

	onlineCount := len(onlineDevices)
	offlineCount := len(offlineDevices)
	notFoundCount := len(notFoundDevices)
	targetNotFoundCount := len(targetNotFoundDevices)
	otherErrCount := len(otherErrDevices)

	voiceQueryDeviceResults := map[string]interface{}{
		"online_count":           onlineCount,
		"offline_count":          offlineCount,
		"not_found_count":        notFoundCount,
		"target_not_found_count": targetNotFoundCount,
		"other_err_count":        otherErrCount,
		"online":                 onlineDevices.GetIDs(),
		"offline":                offlineDevices.GetIDs(),
		"not_found":              notFoundDevices.GetIDs(),
		"target_not_found":       targetNotFoundDevices.GetIDs(),
		"other_err":              otherErrDevices.GetIDs(),
	}
	msg := "VoiceQueryDeviceResults"
	ctxlog.Info(ctx, b.Logger, msg, log.Any("voice_query_device_results", voiceQueryDeviceResults))
	setrace.InfoLogEvent(
		ctx, b.Logger,
		msg,
		log.Any("voice_query_device_results", voiceQueryDeviceResults),
	)

	// if all devices failed - we have to say that
	if totalErrCount := offlineCount + notFoundCount + targetNotFoundCount + otherErrCount; totalErrCount >= len(devices) {
		resultNLG := nlg.QueryMixedError
		switch {
		case offlineCount >= len(devices):
			resultNLG = ErrorCodeNLG(adapter.DeviceUnreachable)
		case notFoundCount >= len(devices):
			resultNLG = ErrorCodeNLG(adapter.DeviceNotFound)
		case targetNotFoundCount >= len(devices):
			resultNLG = nlg.QueryTargetNotFound
		}
		return libmegamind.OutputResponse{NLG: resultNLG}
	}

	// sort for answer
	sort.Sort(model.DevicesSorting(onlineDevices))
	sort.Sort(DeviceCapabilityQueryResultsSorting(deviceCapabilityResults))
	sort.Sort(DevicePropertyQueryResultsSorting(devicePropertyResults))
	for k := range onlineDevices {
		sort.Sort(model.CapabilitiesSorting(onlineDevices[k].Capabilities))
		sort.Sort(model.PropertiesSorting(onlineDevices[k].Properties))
	}

	// reply is based on target
	resultResponse := libmegamind.OutputResponse{NLG: nlg.QueryCannotDo}
	switch searchFor := extractedQuery.SearchFor; searchFor.Target {
	case model.StateTarget:
		resultResponse.NLG = QueryStateNLG(b.Inflector, onlineDevices)
	case model.ModeTarget:
		resultResponse.NLG = QueryAllModesNLG(b.Inflector, onlineDevices)
	case model.CapabilityTarget:
		resultResponse.NLG = QueryCapabilitiesNLG(b.Inflector, deviceCapabilityResults, extractedQuery)
	case model.PropertyTarget:
		resultResponse = QueryPropertiesOutputResponse(request, b.Inflector, devicePropertyResults, extractedQuery)
	}
	return resultResponse
}

func (b *BegemotProcessor) processActionApplyArguments(ctx context.Context, request *scenarios.TScenarioApplyRequest, aaProto *protos.DeviceActionsApplyArguments, origin model.Origin) (*scenarios.TScenarioApplyResponse, error) {
	var applyArguments megamind.DeviceActionsApplyArguments
	applyArguments.FromProto(aaProto)

	analyticsInfo := formAnalyticsInfo(aaProto.Hypotheses, aaProto.SelectedHypotheses)
	if len(applyArguments.ExtractedActions) == 0 {
		return ErrorApplyResponse(ctx, b.Logger, xerrors.New("ExtractedActions is empty"))
	}

	clientInfo := libmegamind.NewClientInfo(request.BaseRequest.ClientInfo)

	isDelayedAction := !applyArguments.ExtractedActions[0].TimeInfo.IsInterval && !applyArguments.ExtractedActions[0].TimeInfo.DateTime.IsZero()
	if isDelayedAction {
		return b.processDelayedDeviceActions(ctx, clientInfo, applyArguments, analyticsInfo, origin)
	}
	if applyArguments.ExtractedActions.HaveScenarioActions() {
		return b.processScenarioActions(ctx, request, clientInfo, applyArguments, analyticsInfo, origin)
	}
	return b.processDeviceActions(ctx, clientInfo, applyArguments, analyticsInfo, origin)
}

func (b *BegemotProcessor) processDelayedDeviceActions(
	ctx context.Context,
	clientInfo libmegamind.ClientInfo,
	applyArguments megamind.DeviceActionsApplyArguments,
	analyticsInfo *AnalyticsInfo,
	origin model.Origin,
) (*scenarios.TScenarioApplyResponse, error) {
	createdTime := applyArguments.ExtractedActions[0].CreatedTime
	scheduledTime := applyArguments.ExtractedActions[0].TimeInfo.DateTime.Time
	scenarioID, err := b.submitDelayedActions(ctx, origin.User.ID, applyArguments.ExtractedActions, createdTime, scheduledTime)
	if err != nil {
		return ErrorApplyResponse(ctx, b.Logger, err)
	}

	callback := GetCancelScenarioLaunchCallback(scenarioID)
	outputResponse := libmegamind.OutputResponse{
		NLG:      nlg.DelayedAction(time.Now(), scheduledTime, clientInfo.GetLocation(model.DefaultTimezone)),
		Callback: &callback,
	}
	return NlgApplyResponse(ctx, b.Logger, outputResponse, analyticsInfo)
}

func (b *BegemotProcessor) processDeviceActions(
	ctx context.Context,
	clientInfo libmegamind.ClientInfo,
	applyArguments megamind.DeviceActionsApplyArguments,
	analyticsInfo *AnalyticsInfo,
	origin model.Origin,
) (*scenarios.TScenarioApplyResponse, error) {
	ctx = contexter.NoCancel(ctx)

	userSettings, err := settings.SettingsFromContext(ctx)
	if err != nil {
		ctxlog.Warn(ctx, b.Logger, "failed to get user settings from context, use empty settings")
		userSettings = settings.UserSettings{}
	}

	timeout, err := timestamp.ComputeTimeout(ctx, MMApplyTimeout)
	if err != nil {
		timeout = time.Millisecond * 1000
	}

	isIntervalAction := applyArguments.ExtractedActions[0].TimeInfo.IsInterval
	if isIntervalAction {
		var cancel context.CancelFunc
		ctx, cancel = context.WithTimeout(ctx, timeout)
		defer cancel()
	}

	deviceActions := convertApplyArgumentsToDeviceActions(applyArguments)
	sideEffects := b.ActionController.SendActionsToDevicesV2(ctx, origin, deviceActions)
	if len(sideEffects.Directives) > 0 {
		if isIntervalAction {
			return b.postProcessIntervalAction(ctx, applyArguments.ExtractedActions, origin.User, sideEffects.Directives, clientInfo, analyticsInfo)
		}
		eaNLG := applyArguments.ExtractedActions.NLG(applyArguments.Devices, clientInfo.IsSmartSpeaker(), userSettings.IoTResponseReactionType())
		outputResponse := libmegamind.OutputResponse{NLG: eaNLG}
		outputResponse.AddDirectives(sideEffects.Directives)
		return NlgApplyResponse(ctx, b.Logger, outputResponse, analyticsInfo)
	}

	select {
	case result := <-sideEffects.Result():
		if isIntervalAction && result.HasSuccessfulActions() {
			return b.postProcessIntervalAction(ctx, applyArguments.ExtractedActions, origin.User, nil, clientInfo, analyticsInfo)
		}
		eaNLG := applyArguments.ExtractedActions.NLG(applyArguments.Devices, clientInfo.IsSmartSpeaker(), userSettings.IoTResponseReactionType())
		return NlgApplyResponse(ctx, b.Logger, b.processSendDeviceActionsResult(ctx, result, eaNLG), analyticsInfo)
	case <-time.After(timeout):
		setrace.InfoLogEvent(
			ctx, b.Logger,
			fmt.Sprintf("timeout was reached after %dms (is interval action: %t)", timeout.Milliseconds(), isIntervalAction),
		)
		if isIntervalAction {
			outputResponse := libmegamind.OutputResponse{NLG: nlg.IntervalActionTimeoutReached}
			return NlgApplyResponse(ctx, b.Logger, outputResponse, analyticsInfo)
		}
		outputResponse := libmegamind.OutputResponse{NLG: nlg.OptimisticOK}
		return NlgApplyResponse(ctx, b.Logger, outputResponse, analyticsInfo)
	}
}

func convertApplyArgumentsToDeviceActions(applyArguments megamind.DeviceActionsApplyArguments) []action.DeviceAction {
	// not in dto/megamind to prevent cycle dependencies
	deviceContainers := applyArguments.ExtractedActions.ToDevices()

	deviceActions := make([]action.DeviceAction, 0, len(deviceContainers))
	deviceMap := applyArguments.Devices.ToMap()
	for _, deviceContainer := range deviceContainers {
		if device, ok := deviceMap[deviceContainer.ID]; ok {
			deviceActions = append(deviceActions, action.NewDeviceAction(device, deviceContainer.Capabilities))
		}
	}

	return deviceActions
}

func (b *BegemotProcessor) processScenarioActions(
	ctx context.Context,
	request *scenarios.TScenarioApplyRequest,
	clientInfo libmegamind.ClientInfo,
	applyArguments megamind.DeviceActionsApplyArguments,
	analyticsInfo *AnalyticsInfo,
	origin model.Origin,
) (*scenarios.TScenarioApplyResponse, error) {
	userSettings, err := settings.SettingsFromContext(ctx)
	if err != nil {
		ctxlog.Warn(ctx, b.Logger, "failed to get user settings from context, use empty settings")
		userSettings = settings.UserSettings{}
	}

	type actionResultMsg struct {
		result scenario.LaunchResults
		err    error
	}
	resultCh := make(chan actionResultMsg, 1)
	noCancelCtx := contexter.NoCancel(ctx)

	timeout, err := timestamp.ComputeTimeout(ctx, MMApplyTimeout)
	if err != nil {
		timeout = time.Millisecond * 1000
	}

	sendActionsCtx := noCancelCtx

	go func() {
		defer func() {
			if r := recover(); r != nil {
				err := xerrors.Errorf("caught panic in apply: %v", r)
				ctxlog.Warn(ctx, b.Logger, fmt.Sprintf("%v", err), log.Any("stacktrace", string(debug.Stack())))
				resultCh <- actionResultMsg{err: err}
			}
		}()
		result := b.sendScenarioActions(sendActionsCtx, applyArguments.Devices, applyArguments.ExtractedActions, origin)
		resultCh <- actionResultMsg{result: result}
	}()

	// we don't have to wait for sendActions result if we know that NLG will not be required
	if extractedActions := applyArguments.ExtractedActions; extractedActions.SilentResponseRequired(applyArguments.Devices, clientInfo.IsSmartSpeaker()) {
		setrace.InfoLogEvent(
			ctx, b.Logger,
			"Silent response is required",
		)
		var animation *libmegamind.LEDAnimation
		deviceWillTalk := extractedActions.HaveSpeakerPhraseAction(clientInfo.DeviceID) || extractedActions.HaveRequestedSpeakerPhraseActions()
		if request.GetBaseRequest().GetInterfaces().GetHasLedDisplay() && !deviceWillTalk {
			animation = &libmegamind.ScenarioOKLedAnimation
		}
		outputResponse := libmegamind.OutputResponse{NLG: libnlg.SilentResponse, Animation: animation}
		return NlgApplyResponse(noCancelCtx, b.Logger, outputResponse, analyticsInfo)
	}

	select {
	case resultMsg := <-resultCh:
		if err := resultMsg.err; err != nil {
			return ErrorApplyResponse(noCancelCtx, b.Logger, err)
		}
		eaNLG := applyArguments.ExtractedActions.NLG(applyArguments.Devices, clientInfo.IsSmartSpeaker(), userSettings.IoTResponseReactionType())
		return NlgApplyResponse(noCancelCtx, b.Logger, b.processSendScenarioActionsResult(ctx, request, resultMsg.result, eaNLG), analyticsInfo)
	case <-time.After(timeout):
		setrace.InfoLogEvent(
			ctx, b.Logger,
			fmt.Sprintf("timeout was reached after %dms ", timeout.Milliseconds()),
		)
		outputResponse := libmegamind.OutputResponse{NLG: nlg.OptimisticOK}
		return NlgApplyResponse(noCancelCtx, b.Logger, outputResponse, analyticsInfo)
	}
}

func (b *BegemotProcessor) sendScenarioActions(ctx context.Context, userDevices []model.Device, extractedActions model.ExtractedActions, origin model.Origin) scenario.LaunchResults {
	actionScenarios := extractedActions.ToScenarios()
	scenariosResultCh := make(chan scenario.LaunchResult, len(actionScenarios))
	var wg sync.WaitGroup
	for i := range actionScenarios {
		wg.Add(1)
		go func(insideCtx context.Context, actionScenario model.Scenario, allUserDevices model.Devices) {
			defer wg.Done()
			defer func() {
				if r := recover(); r != nil {
					err := xerrors.Errorf("caught panic in scenario invoking: %v", r)
					ctxlog.Warn(insideCtx, b.Logger, fmt.Sprintf("%v", err), log.Any("stacktrace", string(debug.Stack())))
					scenariosResultCh <- scenario.LaunchResult{}
				}
			}()
			// FIXME: https://st.yandex-team.ru/IOT-1371
			launch := actionScenario.ToInvokedLaunch(model.VoiceScenarioTrigger{}, timestamp.CurrentTimestampCtx(ctx), allUserDevices)
			if origin.SurfaceParameters.SurfaceType() == model.SpeakerSurfaceType {
				speakerParameters := origin.SurfaceParameters.(model.SpeakerSurfaceParameters)
				requestedDeviceContainer, requestedDeviceFound := allUserDevices.GetDeviceByQuasarExtID(speakerParameters.ID)
				if requestedDeviceFound {
					launch.Steps, _ = launch.Steps.PopulateActionsFromRequestedSpeaker(requestedDeviceContainer)
				}
			}
			launchID, err := b.ScenarioController.StoreScenarioLaunch(ctx, origin.User.ID, launch)
			if err != nil {
				ctxlog.Warnf(ctx, b.Logger, "failed to create scenario launch: %v", err)
				scenariosResultCh <- scenario.LaunchResult{}
				return
			}
			launch.ID = launchID
			if actionScenario.PushOnInvoke {
				if err := b.ScenarioController.SendInvokedLaunchPush(ctx, origin.User, launch); err != nil {
					ctxlog.Warnf(ctx, b.Logger, "failed to send invoke launch push: %v", err)
				}
			}
			scenarioLaunchResult := b.ScenarioController.SendActionsToScenarioLaunch(insideCtx, origin, launch, allUserDevices)
			launch = b.ScenarioController.PopulateScenarioLaunchWithLaunchResult(launch, scenarioLaunchResult)
			if err := b.ScenarioController.UpdateScenarioLaunch(insideCtx, origin, launch); err != nil {
				ctxlog.Warnf(insideCtx, b.Logger, "failed to update scenario launch after invoking: %v", err)
			}
			scenariosResultCh <- scenarioLaunchResult
		}(ctx, actionScenarios[i], userDevices)
	}

	go func() {
		wg.Wait()
		close(scenariosResultCh)
	}()

	var result scenario.LaunchResults
	for scenarioLaunchResult := range scenariosResultCh {
		result = append(result, scenarioLaunchResult)
	}
	return result
}

func (b *BegemotProcessor) processSendScenarioActionsResult(ctx context.Context, request *scenarios.TScenarioApplyRequest, result scenario.LaunchResults, n libnlg.NLG) libmegamind.OutputResponse {
	resultNLG := n
	var animation *libmegamind.LEDAnimation
	if request.GetBaseRequest().GetInterfaces().GetHasLedDisplay() {
		animation = &libmegamind.ScenarioOKLedAnimation
	}
	if result.RequestedSpeakerActionsSent() {
		resultNLG = libnlg.SilentResponse // requested device will talk in response to scenario
		animation = nil                   // and animation should not be shown
	}
	return libmegamind.OutputResponse{
		NLG:       resultNLG,
		Animation: animation,
	}
}

func (b *BegemotProcessor) processSendDeviceActionsResult(ctx context.Context, result action.DevicesResult, n libnlg.NLG) libmegamind.OutputResponse {
	devicesResult := result.Flatten()
	switch {
	case len(devicesResult) == 1:
		deviceResult := devicesResult[0]
		for errorCode, count := range deviceResult.GetErrorCodeCountMap() {
			if count > 0 {
				return libmegamind.OutputResponse{
					NLG: ErrorCodeNLG(errorCode),
				}
			}
		}
	case len(devicesResult) > 1:
		errorCodeCountMap := make(adapter.ErrorCodeCountMap)
		for _, deviceResult := range devicesResult {
			errorCodeCountMap.Add(deviceResult.GetErrorCodeCountMap())
		}
		if totalErrs := errorCodeCountMap.Total(); totalErrs > 0 {
			if totalErrs == len(devicesResult) {
				if len(errorCodeCountMap) == 1 {
					for errorCode := range errorCodeCountMap {
						return libmegamind.OutputResponse{
							NLG: MultipleDevicesErrorCodeNLG(errorCode),
						}
					}
				}
				return libmegamind.OutputResponse{
					NLG: nlg.AllActionsFailedError,
				}
			}
		}
	}
	return libmegamind.OutputResponse{
		NLG: n.UseTextOnly(result.ContainsTextOnlyNLG()),
	}
}

func (b *BegemotProcessor) postProcessIntervalAction(
	ctx context.Context,
	extractedActions model.ExtractedActions,
	user model.User,
	directives []*scenarios.TDirective,
	clientInfo libmegamind.ClientInfo,
	analyticsInfo *AnalyticsInfo,
) (*scenarios.TScenarioApplyResponse, error) {

	devices := extractedActions.ToDevices()

	var err error
	extractedActions, err = extractedActions.InvertCapabilityStates()
	if err != nil {
		return ErrorApplyResponse(ctx, b.Logger, err)
	}

	createdTime := extractedActions[0].CreatedTime
	scheduledTime := extractedActions[0].TimeInfo.EndDateTime.Time // we schedule action for the end of the interval
	launchID, err := b.submitDelayedActions(ctx, user.ID, extractedActions, createdTime, scheduledTime)
	if err != nil {
		return ErrorApplyResponse(ctx, b.Logger, err)
	}

	intervalActionsNLG := customIntervalActionNLG(devices, time.Now(), scheduledTime, clientInfo.GetLocation(model.DefaultTimezone))
	callback := GetCancelScenarioLaunchCallback(launchID)
	outputResponse := libmegamind.OutputResponse{
		NLG:      intervalActionsNLG,
		Callback: &callback,
	}
	outputResponse.AddDirectives(directives)
	return NlgApplyResponse(ctx, b.Logger, outputResponse, analyticsInfo)
}

func customIntervalActionNLG(devices []model.Device, now time.Time, scheduleTime time.Time, userLocation *time.Location) libnlg.NLG {
	hasOnlyOnOffCapabilities := true
	onOffTargetStates := make([]bool, 0)
	for _, device := range devices {
		for _, capability := range device.Capabilities {
			if capability.Type() != model.OnOffCapabilityType {
				hasOnlyOnOffCapabilities = false
				break
			}
			stateValue := capability.State().(model.OnOffCapabilityState).Value
			onOffTargetStates = append(onOffTargetStates, stateValue)
		}
	}

	if len(devices) == 0 {
		return nlg.CommonIntervalAction(now, scheduleTime, userLocation)
	}

	hasSameOnOffTargetState := true
	for i := 1; i < len(onOffTargetStates); i++ {
		if onOffTargetStates[i] != onOffTargetStates[i-1] {
			hasSameOnOffTargetState = false
			break
		}
	}

	if hasOnlyOnOffCapabilities && hasSameOnOffTargetState {
		return nlg.OnOffStateIntervalAction(onOffTargetStates[0], now, scheduleTime, userLocation)
	}
	return nlg.CommonIntervalAction(now, scheduleTime, userLocation)
}

func (b *BegemotProcessor) submitDelayedActions(ctx context.Context, userID uint64, ea model.ExtractedActions, createdTime, scheduledTime time.Time) (string, error) {
	userDevices := ea.ToDevices()
	scenarioDevices := model.ScenarioDevices(ea.ToScenarioDevices())
	stepAction := model.MakeScenarioStepByType(model.ScenarioStepActionsType)
	stepAction.SetParameters(model.ScenarioStepActionsParameters{
		Devices: scenarioDevices.MakeScenarioLaunchDevicesByActualDevices(userDevices),
	})

	launch := model.ScenarioLaunch{
		LaunchTriggerType: model.TimerScenarioTriggerType,
		Steps:             model.ScenarioSteps{stepAction},
		Created:           timestamp.FromTime(createdTime),
		Scheduled:         timestamp.FromTime(scheduledTime),
		Status:            model.ScenarioLaunchScheduled,
	}

	launch.ScenarioName = launch.GetTimerScenarioName()
	launch.Icon = model.ScenarioIcon(launch.ScenarioSteps().AggregateDeviceType())

	return b.ScenarioController.CreateScheduledScenarioLaunch(ctx, userID, launch)
}

func granetProcessorShouldBeUsed(ctx context.Context, megamindExperiments map[string]bool) bool {
	return experiments.EnableGranetProcessors.IsEnabled(ctx) || megamindExperiments[EnableGranetProcessorsExp]
}

func noSmartHomeDevices(userInfo model.UserInfo) bool {
	return (len(userInfo.Devices) == 0 || userInfo.Devices.ContainsOnlySpeakerDevices()) &&
		len(userInfo.Scenarios) == 0 &&
		len(userInfo.Devices.FilterByDeviceTypes(model.DeviceTypes{model.YandexStationMidiDeviceType})) == 0
}

package begemot

import (
	"context"
	"encoding/base64"
	"fmt"

	"github.com/golang/protobuf/proto"
	"golang.org/x/exp/slices"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/megamind"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	libbegemot "a.yandex-team.ru/alice/library/go/begemot"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Client struct {
	begemotClient libbegemot.IClient
	logger        log.Logger
}

var (
	NoHypothesesFoundError = xerrors.New("begemot response has empty AliceIot result")
)

func NewClient(client libbegemot.IClient, logger log.Logger) *Client {
	return &Client{
		begemotClient: client,
		logger:        logger,
	}
}

func (c *Client) iotUserInfoWizExtra(ctx context.Context, userInfo model.UserInfo) (string, error) {
	userInfoProto := userInfo.ToUserInfoProto(ctx)
	userInfoProtoBytes, err := proto.Marshal(userInfoProto)
	if err != nil {
		return "", xerrors.Errorf("can't marshal iot user info: %w", err)
	}
	userInfoBase64 := base64.URLEncoding.EncodeToString(userInfoProtoBytes)
	return fmt.Sprintf("iot_user_info=%s", userInfoBase64), nil
}

func (c *Client) GetHypotheses(ctx context.Context, query string, userInfo model.UserInfo) (scenarios.TBegemotIotNluResult_EHypothesesType, megamind.Hypotheses, error) {
	iotUserInfoWizExtra, err := c.iotUserInfoWizExtra(ctx, userInfo)
	if err != nil {
		return 0, nil, xerrors.Errorf("unable to create begemot request: %w", err)
	}

	request := libbegemot.WizardRequest{
		Text:           query,
		WizardClient:   libbegemot.MegamindWizardClient,
		TopLevelDomain: libbegemot.RuTLD,
		UserLanguage:   libbegemot.RuLang,
		Rules: libbegemot.BegemotRules{
			libbegemot.AliceCustomEntities, libbegemot.AliceIot, libbegemot.AliceRequest, libbegemot.AliceNormalizer, libbegemot.FstTime, libbegemot.FstDatetime, libbegemot.FstDatetimeRange,
		},
		WizExtra: []string{fmt.Sprintf("alice_preprocessing=true;%s", iotUserInfoWizExtra)},
	}
	response, err := c.begemotClient.Wizard(ctx, request)
	if err != nil {
		return 0, nil, xerrors.Errorf("unable to get hypotheses: %w", err)
	}
	if err := response.Rules.AliceIOTResult.Error(); err != nil {
		return 0, nil, xerrors.Errorf("begemot response got error in AliceIot rule: %w", err)
	}
	if response.Rules.AliceIOTResult.IsEmpty() {
		return 0, nil, NoHypothesesFoundError
	}
	nluResult := response.Rules.AliceIOTResult.Result
	ctxlog.Debug(ctx, c.logger, "iot begemot rule result", tools.ProtoJSONLogField("begemot_iot_nlu_result", nluResult))
	hs, err := megamind.NewHypotheses(nluResult.GetTypedHypotheses())
	if err != nil {
		return 0, nil, xerrors.Errorf("error parsing BegemotIotNluResult.TypedHypotheses: %w", err)
	}
	ctxlog.Debug(ctx, c.logger, "successfully extracted typed hypotheses from begemot rule", log.Any("typed_hypotheses", hs))
	return nluResult.HypothesesType, hs, nil
}

func ScenarioIDByPushText(ctx context.Context, logger log.Logger, begemotClient IClient, pushText string, allScenarioIDs []string, userInfo model.UserInfo) (string, error) {
	_, hypotheses, err := begemotClient.GetHypotheses(ctx, tools.Standardize(pushText), userInfo)
	if err != nil {
		switch {
		case xerrors.Is(err, NoHypothesesFoundError):
			return "", nil
		default:
			// can't validate scenarios due to unexpected begemot error
			ctxlog.Warnf(ctx, logger, "unexpected error in begemotClient.GetHypotheses: %v", err)
			return "", &model.ScenarioTextServerActionInternalValidationError{}
		}
	}
	ctxlog.Debug(
		ctx, logger,
		"searching for scenarios calling other scenarios",
		log.Any("scenarios_in_hypotheses", hypotheses.ScenarioIDs()),
		log.Any("all_scenarios", allScenarioIDs),
	)
	for _, h := range hypotheses {
		if scenarioID := h.ScenarioID; len(scenarioID) > 0 && slices.Contains(allScenarioIDs, scenarioID) {
			// scenario will trigger other scenario - this is prohibited
			ctxlog.Debugf(ctx, logger, "found recursive scenario call - pushText `%s` would call scenario %s", pushText, scenarioID)
			return scenarioID, nil
		}
	}
	return "", nil
}

func ValidatePushText(ctx context.Context, logger log.Logger, begemotClient IClient, pushText string, allScenarioIDs []string, userInfo model.UserInfo) error {
	scenarioID, err := ScenarioIDByPushText(ctx, logger, begemotClient, pushText, allScenarioIDs, userInfo)
	if err != nil {
		return err
	}
	if scenarioID != "" {
		return &model.ScenarioTextServerActionNameError{}
	}
	return nil
}

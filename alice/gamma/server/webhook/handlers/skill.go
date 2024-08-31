package handlers

import (
	"time"

	"github.com/golang/protobuf/proto"
	"golang.org/x/xerrors"
	"google.golang.org/grpc"

	skillsAPI "a.yandex-team.ru/alice/gamma/sdk/api"
	"a.yandex-team.ru/alice/gamma/server/log"
	"a.yandex-team.ru/alice/gamma/server/skills"
	"a.yandex-team.ru/alice/gamma/server/storage"
	webhookAPI "a.yandex-team.ru/alice/gamma/server/webhook/api"
)

type SkillHandler struct {
	provider skills.Provider
	storage  *storage.ProxyStorage
}

func (handler *SkillHandler) Handle(
	ctx log.LoggingContext, skillID string, request *webhookAPI.Request,
) (response *webhookAPI.Response, err error) {
	skillInfo, err := handler.provider.GetSkill(ctx, skillID)
	if err != nil {
		return nil, err
	}
	protoRequest, err := webhookAPI.FromWebhookRequest(skillInfo.ID, request)
	if err != nil {
		return nil, err
	}
	var protoResponse *skillsAPI.SkillResponse

	session := request.Session
	state, err := handler.initState(ctx, skillInfo.ID, session.SessionID, session.UserID, session.MessageID, request.State.SessionState)
	if err != nil {
		if !xerrors.Is(err, storage.AlreadyCommittedError) {
			return nil, err
		}
		protoResponse, err = handler.handleCached(ctx, state, protoRequest)
	} else {
		defer func() {
			if finalizeErr := handler.finalizeState(ctx, state); finalizeErr != nil {
				if xerrors.Is(finalizeErr, storage.AlreadyCommittedError) {
					ctx.Logger.Warnf("Failed to finalize state: already committed transaction")
				} else {
					ctx.Logger.Errorf("Failed to finalize state: %+v", finalizeErr)
					if err == nil {
						err = finalizeErr
					}
				}
			}
		}()

		protoResponse, err = handler.handle(ctx, skillInfo, state, protoRequest)
		if err != nil {
			return nil, err
		}
	}
	return webhookAPI.ToWebhookResponse(skillID, protoResponse)
}

func unmarshalResponse(response []byte, session *skillsAPI.Session, protoResponse *skillsAPI.SkillResponse) error {
	protoResponse.Session = session
	protoResponse.Response = new(skillsAPI.ResponseBody)
	return proto.Unmarshal(response, protoResponse.Response)
}

func marshalResponse(protoResponse *skillsAPI.SkillResponse) ([]byte, error) {
	return proto.Marshal(protoResponse.GetResponse())
}

func (handler *SkillHandler) handleCached(
	ctx log.LoggingContext, state *skillState, request *skillsAPI.SkillRequest,
) (*skillsAPI.SkillResponse, error) {
	ctx.Logger.Info("Already committed transaction. Restoring response")
	storedResponse := state.StorageTransaction.GetResponse()
	response := skillsAPI.SkillResponse{}
	if err := unmarshalResponse(storedResponse, request.Session, &response); err != nil {
		return nil, xerrors.Errorf("malformed saved response: %w", err)
	}
	return &response, nil
}

func (handler *SkillHandler) handle(
	ctx log.LoggingContext, skillInfo *skills.Info, state *skillState, request *skillsAPI.SkillRequest,
) (response *skillsAPI.SkillResponse, err error) {
	connection, err := grpc.Dial(skillInfo.Addr, grpc.WithInsecure())
	if err != nil {
		return nil, err
	}
	defer func() {
		if err := connection.Close(); err != nil {
			ctx.Logger.Errorf("Closing connection failed: %+v", err)
		}
	}()

	client := skillsAPI.NewSkillClient(connection)

	storageValue := state.StorageTransaction.GetItem()
	request.State = &skillsAPI.State{
		Storage: storageValue,
	}

	handleTime := time.Now()

	response, err = client.Handle(ctx, request)

	ctx.Logger.Infof("Skill Handle took %s", time.Since(handleTime))
	if err != nil {
		return nil, err
	}

	var item storage.Item = response.State.GetStorage()

	state.StorageTransaction.SetItem(item)
	storedResponse, err := marshalResponse(response)
	if err != nil {
		ctx.Logger.Errorf("Failed to save response %+v", err)
	} else {
		state.StorageTransaction.SetResponse(storedResponse)
	}
	state.IsOk = true
	ctx.Logger.Debugf("New skill state: %s", item)
	return response, nil
}

func CreateHandler(provider skills.Provider, storage_ *storage.ProxyStorage) *SkillHandler {
	return &SkillHandler{provider: provider, storage: storage_}
}

var InvalidStateError = xerrors.New("invalid state")

type skillState struct {
	SkillID            string
	UserID             string
	SessionID          string
	MessageID          int64
	StorageTransaction storage.Transaction
	IsOk               bool
}

func (handler *SkillHandler) initState(
	ctx log.LoggingContext, skillID string, sessionID string, userID string, messageID int64, state []byte,
) (*skillState, error) {
	var err error
	session := skillState{
		SkillID:   skillID,
		SessionID: sessionID,
		MessageID: messageID,
		UserID:    userID,
		IsOk:      false,
	}

	session.StorageTransaction, err = handler.storage.StartTransaction(ctx, skillID, sessionID, userID, messageID, state)

	if err != nil {
		if xerrors.Is(err, storage.AlreadyCommittedError) {
			return &session, err
		}
		return nil, err
	}

	return &session, nil
}

func (handler *SkillHandler) finalizeState(ctx log.LoggingContext, state *skillState) error {
	if handler == nil {
		return InvalidStateError
	}

	if state.IsOk {
		return handler.storage.CommitTransaction(ctx, state.StorageTransaction)
	} else {
		return handler.storage.RollbackTransaction(ctx, state.StorageTransaction)
	}
}

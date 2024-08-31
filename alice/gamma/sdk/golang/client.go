package sdk

import (
	"context"
	"encoding/json"
	"flag"
	"fmt"
	stdLog "log"
	"net"

	"go.uber.org/zap"
	"go.uber.org/zap/zapcore"
	"golang.org/x/xerrors"
	"google.golang.org/grpc"

	"a.yandex-team.ru/alice/gamma/sdk/api"
)

var BadRequestError = xerrors.New("bad request")

type Context interface {
	IsNewSession() bool
	GetState() interface{}
	Match(request *Request, patterns []Pattern, extractor Extractor) ([]Hypothesis, error)
}

type SkillContext struct {
	ctx        context.Context
	connection *clientConnection
	session    *api.Session
}

func setupLogger(jsonFormat bool, level string) *zap.SugaredLogger {
	var cfg zap.Config
	if jsonFormat {
		cfg = zap.NewProductionConfig()
		cfg.EncoderConfig.EncodeTime = zapcore.ISO8601TimeEncoder
		cfg.Encoding = "json"
	} else {
		cfg = zap.NewDevelopmentConfig()
		cfg.Encoding = "console"
	}
	cfg.Level = zap.NewAtomicLevel()
	if err := cfg.Level.UnmarshalText([]byte(level)); err != nil {
		stdLog.Panicf("Bad log level format: %+v", err)
	}

	logger_, err := cfg.Build()
	if err != nil {
		stdLog.Panicf("Failed to init logger: %+v", err)
	}
	return logger_.Sugar()
}

type Options interface {
	GetOptions() []ValueOption
}

type DefaultOptions struct {
}

func (DefaultOptions) GetOptions() []ValueOption {
	return nil
}

type SkillOption interface {
	Set(string) error
	String() string
}

type ValueOption struct {
	Name  string
	Usage string
	Value SkillOption
}

func StartSkill(skill Skill, options Options) {
	for _, option := range options.GetOptions() {
		flag.Var(option.Value, option.Name, option.Usage)
	}

	var (
		portPtr          int
		apiAddr          string
		useJSONLogFormat bool
		logLevel         string
	)

	flag.IntVar(&portPtr, "port", 8001, "Listening port")
	flag.StringVar(&apiAddr, "sdk-addr", "localhost:8002", "sdk api address")
	flag.BoolVar(&useJSONLogFormat, "log-json", false, "Format logger output as json")
	flag.StringVar(&logLevel, "log-level", "DEBUG", "One of DEBUG,INFO,WARN,ERROR")

	flag.Parse()

	logger := setupLogger(useJSONLogFormat, logLevel)
	defer func() {
		_ = logger.Sync()
	}()

	logger.Info("Setting up skill")
	if err := skill.Setup(logger, options); err != nil {
		logger.Fatalf("Failed to setup skill: %+v", err)
	}
	defer func() {
		logger.Info("Finalizing skill")
		if err := skill.Finalize(logger); err != nil {
			logger.Fatalf("Failed to finalize skill: %+v", err)
		}
	}()

	logger.Infof("Starting skill on %d", portPtr)

	apiClient := client{apiAddr: apiAddr}
	listener, err := net.Listen("tcp", fmt.Sprintf(":%d", portPtr))
	if err != nil {
		logger.Fatalf("Failed to start server on port %d: %+v", portPtr, err)
	}
	grpcServer := grpc.NewServer()
	api.RegisterSkillServer(grpcServer, &skillHandler{
		apiClient: &apiClient,
		skill:     skill,
		logger:    logger,
	})

	logger.Fatal(
		grpcServer.Serve(listener),
	)
}

func metaFromProto(protoMeta *api.Meta) *Meta {
	if protoMeta != nil {
		return &Meta{
			ClientID: protoMeta.ClientId,
			Locale:   protoMeta.Locale,
			Timezone: protoMeta.Timezone,
			Interfaces: Interfaces{
				Screen: protoMeta.GetInterfaces().GetScreen(),
			},
		}
	}
	return &Meta{}
}

func requestFromProto(protoRequest *api.RequestBody) (request *Request, err error) {
	if protoRequest == nil {
		return nil, BadRequestError
	}

	request = &Request{
		Command:           protoRequest.Command,
		OriginalUtterance: protoRequest.OriginalUtterance,
		Type:              protoRequest.Type,
	}

	if protoRequest.Payload != nil {
		err = json.Unmarshal(protoRequest.Payload, &request.Payload)
		if err != nil {
			return nil, err
		}
	}
	if request.Nlu, err = nluFromProto(protoRequest.Nlu); err != nil {
		return nil, err
	}
	return request, nil
}

func nluFromProto(protoNlu *api.Nlu) (*Nlu, error) {
	result := &Nlu{}
	if protoNlu == nil {
		return result, nil
	}
	result.Entities = make([]Entity, len(protoNlu.Entities))
	var err error
	for i, entity := range protoNlu.Entities {
		result.Entities[i], err = entityFromProto(entity)
		if err != nil {
			return nil, err
		}
	}
	result.Tokens = make([]string, len(protoNlu.Tokens))
	copy(result.Tokens, protoNlu.Tokens)
	return result, nil
}

func entityFromProto(entity *api.Entity) (Entity, error) {
	result := Entity{
		Start: entity.Start,
		End:   entity.End,
		Type:  entity.Type,
	}
	result.Value = entity.ProtoValue
	return result, nil
}

func (entity *Entity) toProto() (result *api.Entity, err error) {
	result = &api.Entity{
		Start: entity.Start,
		End:   entity.End,
		Type:  entity.Type,
	}
	result.Value, err = json.Marshal(entity.Value)
	return result, err
}

func stateFromProto(state interface{}, protoState *api.State) error {
	if protoState.GetStorage() != nil {
		if err := json.Unmarshal(protoState.Storage, &state); err != nil {
			return err
		}
	}
	return nil
}

func stateToProto(state interface{}) (*api.State, error) {
	storage, err := json.Marshal(&state)
	return &api.State{
		Storage: storage,
	}, err
}

type skillHandler struct {
	apiClient *client
	skill     Skill
	logger    *zap.SugaredLogger
}

func (handler *skillHandler) Handle(ctx context.Context, protoRequest *api.SkillRequest) (*api.SkillResponse, error) {
	session := protoRequest.GetSession()
	if session == nil {
		return nil, BadRequestError
	}

	logger := handler.logger.With(
		zap.String("uuid", session.UserId),
		zap.String("session_id", session.SessionId),
		zap.String("skill_id", session.SkillId),
		zap.Int64("message_id", session.MessageId),
	)

	connection, err := handler.apiClient.connect(logger)
	if err != nil {
		logger.Fatal("Can't connect to sdk")
		return nil, err
	}
	defer func() {
		if err := connection.close(); err != nil {
			logger.Errorf("Failed to close connection: %+v", err)
		}
	}()

	baseContext := &SkillContext{
		ctx:        ctx,
		connection: connection,
		session:    session,
	}
	skillContext := handler.skill.CreateContext(baseContext)
	err = stateFromProto(skillContext.GetState(), protoRequest.State)
	if err != nil {
		logger.Errorf("Can't init skill state: %+v", err)
		return nil, err
	}

	response := api.SkillResponse{
		Session: session,
	}
	request, err := requestFromProto(protoRequest.Request)
	if err != nil {
		logger.Errorf("Failed to deserialize request: %+v", protoRequest.GetRequest())
		return nil, err
	}
	logger.Infof("Received request %+v", request)

	responseBody, err := handler.skill.Handle(
		logger,
		skillContext,
		request,
		metaFromProto(protoRequest.Meta),
	)
	if err != nil {
		logger.Errorf("Skill handling failed: %+v", err)
		return nil, err
	}
	logger.Debugf("Skill response %+v", responseBody)

	response.Response, err = responseBody.toProto()
	if err != nil {
		logger.Error("Error while serializing response")
		return nil, err
	}

	response.State, err = stateToProto(skillContext.GetState())
	if err != nil {
		return nil, err
	}

	return &response, nil
}

type client struct {
	apiAddr string
}

type clientConnection struct {
	logger     Logger
	connection *grpc.ClientConn
	client     api.SdkClient
}

func (apiClient *client) connect(logger Logger) (*clientConnection, error) {
	connection, err := grpc.Dial(apiClient.apiAddr, grpc.WithInsecure())
	if err != nil {
		return nil, err
	}
	client := api.NewSdkClient(connection)
	return &clientConnection{
		logger:     logger,
		connection: connection,
		client:     client,
	}, nil
}

func (connection *clientConnection) close() error {
	return connection.connection.Close()
}

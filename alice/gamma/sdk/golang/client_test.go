package sdk

import (
	"context"
	"encoding/json"
	"net"
	"testing"

	"github.com/stretchr/testify/require"
	"go.uber.org/zap"
	"google.golang.org/grpc"

	"a.yandex-team.ru/alice/gamma/sdk/api"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/test/assertpb"
)

type emptySdkMock struct{}

func (*emptySdkMock) Match(context.Context, *api.MatchRequest) (*api.MatchResponse, error) {
	return nil, xerrors.New("not implemented")
}

type testSkill struct {
	t *testing.T
	BaseSkill
}

func (skill *testSkill) CreateContext(context Context) Context {
	return &MyContext{Context: context, Value: 2}
}

type MyContext struct {
	Context
	Value int
}

func (context *MyContext) GetState() interface{} {
	return &context.Value
}

func (skill *testSkill) Handle(log Logger, context Context, request *Request, meta *Meta) (*Response, error) {
	ctx, ok := context.(*MyContext)
	if !ok {
		skill.t.Errorf("invalid stored context")
	}

	ctx.Value += 40
	return &Response{
		Text: "Test",
	}, nil
}

func runTestServer(t *testing.T, server api.SdkServer) string {
	listener, err := net.Listen("tcp", ":0")
	require.NoError(t, err)
	sdkGrpcServer := grpc.NewServer()

	api.RegisterSdkServer(
		sdkGrpcServer,
		server,
	)

	go testGrpcListener(t, sdkGrpcServer, listener)
	return listener.Addr().String()
}

func testGrpcListener(t *testing.T, s *grpc.Server, l net.Listener) {
	err := s.Serve(l)
	t.Errorf("grpc server error: %v", err)
}

func testMarshalState(t *testing.T, value int) []byte {
	storage, err := json.Marshal(value)
	if err != nil {
		t.Fatal(err)
	}
	return storage
}

func testRequest(t *testing.T, ctx context.Context, client api.SkillClient, req *api.SkillRequest) *api.SkillResponse {
	resp, err := client.Handle(ctx, req)
	if err != nil {
		t.Fatal(err)
	}
	return resp
}

func testSimpleRequestWithState(t *testing.T, value int) *api.SkillRequest {
	return &api.SkillRequest{
		Request: &api.RequestBody{},
		State: &api.State{
			Storage: testMarshalState(t, value),
		},
		Session: &api.Session{},
	}
}

func TestClient(t *testing.T) {
	addr := runTestServer(t, &emptySdkMock{})

	apiClient := client{apiAddr: addr}

	listener, err := net.Listen("tcp", ":0")
	require.NoError(t, err)
	clientGrpcServer := grpc.NewServer()
	api.RegisterSkillServer(
		clientGrpcServer,
		&skillHandler{
			apiClient: &apiClient,
			skill:     &testSkill{t: t},
			logger:    zap.NewNop().Sugar(),
		},
	)
	go testGrpcListener(t, clientGrpcServer, listener)

	connection, _ := grpc.Dial(listener.Addr().String(), grpc.WithInsecure())
	client := api.NewSkillClient(connection)

	resp := testRequest(t, context.Background(), client, testSimpleRequestWithState(t, 2))
	assertpb.Equal(t, &api.ResponseBody{
		Text: "Test",
	}, resp.GetResponse())
	assertpb.Equal(t, &api.State{
		Storage: testMarshalState(t, 42),
	}, resp.GetState())
}

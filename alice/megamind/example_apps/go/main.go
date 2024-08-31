package main

import (
	"fmt"
	"io/ioutil"
	"net/http"

	"github.com/golang/protobuf/proto"
	"github.com/labstack/echo/v4"
	"github.com/labstack/echo/v4/middleware"

	"a.yandex-team.ru/alice/megamind/protos/scenarios"

	"a.yandex-team.ru/alice/megamind/example_apps/go/apps"
)

func CheckStatus(ctx echo.Context) error {
	return ctx.String(http.StatusOK, "Ok")
}

type Requester interface {
	GetMessage() proto.Message
	Handle() (proto.Message, error)
}

type RunRequester struct {
	request scenarios.TScenarioRunRequest
	app     apps.RunApp
}

func (r *RunRequester) GetMessage() proto.Message {
	return &r.request
}

func (r *RunRequester) Handle() (proto.Message, error) {
	return r.app.OnRunRequest(&r.request)
}

type ApplyRequester struct {
	request scenarios.TScenarioApplyRequest
	app     apps.ApplyApp
}

func (r *ApplyRequester) GetMessage() proto.Message {
	return &r.request
}

func (r *ApplyRequester) Handle() (proto.Message, error) {
	return r.app.OnApplyRequest(&r.request)
}

type CommitRequester struct {
	request scenarios.TScenarioApplyRequest
	app     apps.CommitApp
}

func (r *CommitRequester) GetMessage() proto.Message {
	return &r.request
}

func (r *CommitRequester) Handle() (proto.Message, error) {
	return r.app.OnCommitRequest(&r.request)
}

func requestWrapper(ctx echo.Context, requester Requester) error {
	data, err := ioutil.ReadAll(ctx.Request().Body)
	defer func() {
		err = ctx.Request().Body.Close()
	}()
	if err != nil {
		return err
	}
	if err := proto.Unmarshal(data, requester.GetMessage()); err != nil {
		fmt.Printf("Unable to unmarshal request: %+v\n", err)
		ctx.Error(err)
		return err
	}
	fmt.Printf("Request:\n%+v\n", requester.GetMessage())
	response, err := requester.Handle()
	if err != nil {
		fmt.Printf("Unable to handle request: %+v\n", err)
		ctx.Error(err)
		return err
	}
	fmt.Printf("Response:\n%+v\n", response)
	body, err := proto.Marshal(response)
	if err != nil {
		fmt.Printf("Unable to marshal response: %+v\n", err)
		ctx.Error(err)
		return err
	}
	return ctx.Blob(http.StatusOK, "application/protobuf", body)
}

func subscribe(e *echo.Echo, path string, app apps.App) {
	if app, ok := app.(apps.RunApp); ok {
		onRun := func(ctx echo.Context) (err error) {
			return requestWrapper(ctx, &RunRequester{app: app})
		}
		e.POST(path+"/run", onRun)
	}
	if app, ok := app.(apps.ApplyApp); ok {
		onApply := func(ctx echo.Context) (err error) {
			return requestWrapper(ctx, &ApplyRequester{app: app})
		}
		e.POST(path+"/apply", onApply)
	}
	if app, ok := app.(apps.CommitApp); ok {
		onCommit := func(ctx echo.Context) (err error) {
			return requestWrapper(ctx, &CommitRequester{app: app})
		}
		e.POST(path+"/commit", onCommit)
	}
}

func main() {
	server := echo.New()

	server.Use(middleware.Logger())
	server.Use(middleware.Recover())

	cardsProvider := apps.CardsProvider{}
	stateChecker := apps.StateChecker{}
	applyChecker := apps.ApplyChecker{}
	stackEngine := apps.StackEngine{}
	commit := apps.Commit{}
	memento := apps.MementoApp{}

	server.GET("/ping", CheckStatus)

	subscribe(server, "/apply", &applyChecker)
	subscribe(server, "/cards", &cardsProvider)
	subscribe(server, "/state", &stateChecker)
	subscribe(server, "/commit", &commit)
	subscribe(server, "/stackengine", &stackEngine)
	subscribe(server, "/memento", &memento)

	server.Logger.Fatal(server.Start(":8000"))
}

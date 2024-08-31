package timemachine

import (
	"context"
	"fmt"
	"io/ioutil"
	"math/rand"
	"net/http"
	"net/http/httptest"
	"os"
	"testing"
	"time"

	"github.com/stretchr/testify/suite"

	btest "a.yandex-team.ru/alice/iot/bulbasaur/testing"
	"a.yandex-team.ru/alice/iot/time_machine/tasks"
	wtesting "a.yandex-team.ru/alice/iot/time_machine/testing"
	"a.yandex-team.ru/alice/library/go/pgclient"
	"a.yandex-team.ru/alice/library/go/queue"
	"a.yandex-team.ru/alice/library/go/queue/pgbroker"
	alicetvm "a.yandex-team.ru/alice/library/go/tvm"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/yandex/tvm"
)

const (
	dbHost         = "localhost"
	dbPort         = 10123
	dbUsername     = "time_machine"
	dbUserpassword = "passwd"

	fakeBulbasaurTvmClientID = tvm.ClientID(100500)
)

type TestSuite struct {
	suite.Suite
	embeddedPG *wtesting.EmbeddedPG
}

func TestWorker(t *testing.T) {
	rand.Seed(time.Now().UTC().UnixNano())
	suite.Run(t, &TestSuite{embeddedPG: &wtesting.EmbeddedPG{
		Host: dbHost,
		Port: dbPort,
	}})
}

func (s *TestSuite) SetupSuite() {
	if err := s.embeddedPG.StartPostgres(); err != nil {
		s.FailNow("Failed to start postgres", "Postgres path: %s, error: %v", s.embeddedPG.PostgresDir, err)
	}
	if err := s.embeddedPG.CreateUser(dbUsername, dbUserpassword); err != nil {
		s.FailNow(err.Error())
	}
}

func (s *TestSuite) TearDownSuite() {
	if err := s.embeddedPG.StopPostgres(); err != nil {
		s.FailNow(err.Error())
	}
}

func (s *TestSuite) RunTest(name string, subtest func(worker *queue.Worker, q *queue.Queue)) {
	dbname := fmt.Sprintf("time_machine_db_%d", rand.Uint32())

	if err := s.embeddedPG.CreateDatabase(dbname); err != nil {
		s.FailNow(err.Error())
	}

	yaMakeRun := os.Getenv("YA_MAKE_TEST_RUN")
	var dbFolderPath string
	if yaMakeRun != "" {
		dbFolderPath = "db" // should be synced with ya.make file
	} else {
		dbFolderPath = "../../../library/go/queue/pgbroker/db"
	}

	if err := s.embeddedPG.ApplyMigrations(dbFolderPath, dbname); err != nil {
		s.FailNow(err.Error())
	}

	client, err := pgclient.NewPGClient([]string{dbHost}, dbPort, dbname, dbUsername, dbUserpassword, &nop.Logger{})
	if err != nil {
		s.FailNow("Can't connect to PG client", err)
	}

	logger := btest.NopLogger()

	broker := pgbroker.NewBroker(client)
	q := queue.NewQueue(broker)

	tvmTool := alicetvm.ClientMock{
		Logger: logger,
		ServiceTicketsByID: map[tvm.ClientID]string{
			fakeBulbasaurTvmClientID: "service-ticket",
		},
	}

	handler := tasks.NewHTTPCallbackHandler(logger, tvmTool, nil)
	err = q.RegisterTask(tasks.TimeMachineHTTPCallbackTask, nil) // TODO: add retry and policy here
	if err != nil {
		s.FailNow("Failed to register task", err)
	}
	err = q.RegisterHandler(tasks.TimeMachineHTTPCallbackTask, handler.Handle)
	if err != nil {
		s.FailNow("Failed to register handler", err)
	}

	options := []queue.WorkerOption{queue.WithProcessNum(1), queue.WithChunkSize(5)}
	worker := q.NewWorker(tasks.TimeMachineHTTPCallbackTask, logger, options...)

	s.Run(name, func() { subtest(worker, q) })
}

func (s *TestSuite) TestHttpCallbackTask() {
	s.RunTest("HttpCallbackTasks", func(worker *queue.Worker, q *queue.Queue) {
		expectedPayload := []byte("hello, world!")
		var actualPayload []byte

		isProcessed := make(chan bool)
		defer close(isProcessed)

		ts := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			actualPayload, _ = ioutil.ReadAll(r.Body)
			defer func() {
				_ = r.Body.Close()
			}()
			isProcessed <- true
		}))
		defer ts.Close()

		randomUserID := "100500"
		ctx := context.Background()

		payload := tasks.TimeMachineHTTPCallbackPayload{
			Method:       http.MethodPost,
			URL:          ts.URL,
			Headers:      nil,
			RequestBody:  expectedPayload,
			ServiceTvmID: fakeBulbasaurTvmClientID,
		}

		task, err := queue.NewTask(tasks.TimeMachineHTTPCallbackTask, randomUserID, payload, time.Now())
		s.NoError(err)

		err = q.SubmitTasks(ctx, []queue.Task{task})
		s.NoError(err)

		t := time.NewTimer(10 * time.Second)
		go func() {
			select {
			case <-t.C:
			case <-isProcessed:
			}
			worker.Stop()
		}()

		err = worker.Launch()
		s.NoError(err)

		s.Equal(expectedPayload, actualPayload)
	})
}

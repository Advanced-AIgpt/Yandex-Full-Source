package pgbroker_test

import (
	"context"
	"fmt"
	"strconv"
	"time"

	"github.com/go-resty/resty/v2"

	"a.yandex-team.ru/alice/library/go/pgclient"
	"a.yandex-team.ru/alice/library/go/queue"
	"a.yandex-team.ru/alice/library/go/queue/pgbroker"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/core/log/zap"
)

type PayloadExample struct {
	httpURL string
}

const taskName = "HttpCallbackHandler"

func taskHandler(ctx context.Context, payload PayloadExample) error {
	r := resty.New().NewRequest().SetContext(ctx)
	data, err := r.Get(payload.httpURL)
	if err != nil {
		return err
	}

	fmt.Println(data)
	return nil
}

func Example_submitTask() {
	hosts := []string{"host1.db.yandex.net", "host2.db.yandex.net", "host3.db.yandex.net"}
	c, err := pgclient.NewPGClient(hosts, 6432, "db", "user", "password", &nop.Logger{})
	if err != nil {
		panic(err.Error())
	}
	b := pgbroker.NewBroker(c)
	q := queue.NewQueue(b)

	err = q.RegisterTask(taskName, &queue.SimpleRetryPolicy{Count: 10, Delay: queue.LinearDelay(1 * time.Minute)})
	if err != nil {
		panic("Can't register task")
	}

	var userID uint64 = 123456789
	taskID, err := q.SubmitTask(context.Background(), strconv.FormatUint(userID, 10), taskName, "", PayloadExample{httpURL: "http://ya.ru"}, time.Now())
	fmt.Printf("Task submitted, id: %s", taskID)
}

func Example_workerWithPGBroker() {
	logger, err := zap.New(zap.ConsoleConfig(log.DebugLevel))
	if err != nil {
		panic("Can't create logger")
	}

	hosts := []string{"host1.db.yandex.net", "host2.db.yandex.net", "host3.db.yandex.net"}
	c, err := pgclient.NewPGClient(hosts, 6432, "db", "user", "password", &nop.Logger{})
	if err != nil {
		panic(err.Error())
	}

	b := pgbroker.NewBroker(c)
	q := queue.NewQueue(b)

	err = q.RegisterTask(taskName, &queue.SimpleRetryPolicy{Count: 10, Delay: queue.LinearDelay(1 * time.Minute)})
	if err != nil {
		panic(err.Error())
	}
	err = q.RegisterHandler(taskName, taskHandler)
	if err != nil {
		panic(err.Error())
	}

	w1 := q.NewWorker(taskName, logger, queue.WithChunkSize(10000), queue.WithProcessNum(2000))
	if err = w1.Launch(); err != nil {
		logger.Errorf("Worker stopped with error: %v", err)
	}
}

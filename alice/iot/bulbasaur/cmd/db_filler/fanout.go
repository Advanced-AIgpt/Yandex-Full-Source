package main

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"context"
	"sync"
)

func producer(iters int) <-chan int {
	c := make(chan int)
	go func() {
		for i := 0; i < iters; i++ {
			c <- i
		}
		close(c)
	}()
	return c
}

func consumer(ctx context.Context, client *db.DBClient, cin <-chan int, wg *sync.WaitGroup) {
	defer wg.Done()
	for range cin {
		if err := createUser(ctx, client); err != nil {
			logger.Warn(err.Error())
		}
	}
}

func broadcastTask(ch <-chan int, size int) []chan int {
	cs := make([]chan int, size)
	for i := range cs {
		cs[i] = make(chan int)
	}

	go func() {
		for i := range ch {
			for _, c := range cs {
				c <- i
			}
		}
		for _, c := range cs {
			close(c)
		}
	}()
	return cs
}

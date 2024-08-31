package main

import "context"

type IDataSource interface {
	StreamUsersID(ctx context.Context, usersCh chan string)
}

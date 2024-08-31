package libapphost

import (
	"context"
	"fmt"
	"os"
	"runtime/debug"

	grpc_recovery "github.com/grpc-ecosystem/go-grpc-middleware/recovery"
	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"

	"a.yandex-team.ru/alice/library/go/setrace"
	"a.yandex-team.ru/apphost/api/service/go/apphost"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

func DefaultGRPCRecoverOptions(logger log.Logger) []grpc.ServerOption {
	recoverFunc := func(ctx context.Context, rvr interface{}) (err error) {
		if logger != nil {
			stack := string(debug.Stack())
			ctxlog.Errorf(ctx, logger, "%+v %s", rvr, stack)
		} else {
			_, _ = fmt.Fprintf(os.Stderr, "panic: %+v\n", rvr)
			debug.PrintStack()
		}
		return status.Errorf(codes.Unknown, "panic in grpc handler: %v", rvr)
	}
	grpcRecoveryOpts := []grpc_recovery.Option{
		grpc_recovery.WithRecoveryHandlerContext(recoverFunc),
	}
	opts := []grpc.ServerOption{
		grpc.StreamInterceptor(grpc_recovery.StreamServerInterceptor(grpcRecoveryOpts...)),
		grpc.UnaryInterceptor(grpc_recovery.UnaryServerInterceptor(grpcRecoveryOpts...)),
	}
	return opts
}

func RecovererMiddleware(logger log.Logger) apphost.Middleware {
	return func(next apphost.Handler) apphost.Handler {
		return apphost.HandlerFunc(func(ctx apphost.Context) (err error) {
			defer func() {
				goCtx := ctx.Context()
				if rvr := recover(); rvr != nil {
					if logger != nil {
						stack := string(debug.Stack())
						ctxlog.Errorf(goCtx, logger, "%+v %s", rvr, stack)
						setrace.BacktraceLogEvent(goCtx, logger, fmt.Sprintf("%v", rvr), stack)
					} else {
						_, _ = fmt.Fprintf(os.Stderr, "panic: %+v\n", rvr)
						debug.PrintStack()
					}
					err = apphost.RequestErrorf("unhandled error: %v", rvr)
				}
			}()
			return next.ServeAppHost(ctx)
		})
	}
}

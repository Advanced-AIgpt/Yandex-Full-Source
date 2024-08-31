package interceptor

import (
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"strconv"
	"time"

	"github.com/gofrs/uuid"

	"a.yandex-team.ru/alice/amelie/internal/db"
	"a.yandex-team.ru/alice/amelie/internal/model"
	"a.yandex-team.ru/alice/amelie/pkg/telegram"
	"a.yandex-team.ru/alice/library/go/setrace"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

const (
	sessionSaveTimeout = time.Millisecond * 300
	sessionLoadTimeout = time.Millisecond * 300
)

type SessionInterceptor struct {
	db            db.DB
	logger        log.Logger
	serviceLogger log.Logger
}

func (s *SessionInterceptor) Intercept(ctx context.Context, bot telegram.Bot, eventType telegram.EventType, event interface{},
	next telegram.NextInterceptorDelegate) error {
	meta := telegram.GetEventMeta(ctx)
	if meta.ChatID == 0 {
		return fmt.Errorf("invalid chatID")
	}
	sessionID := strconv.FormatInt(meta.ChatID, 10)
	session, err := s.loadSession(ctx, sessionID)
	if err != nil {
		ctxlog.Errorf(ctx, s.serviceLogger, "Session loading error: %v", err)
		return fmt.Errorf("session saving error: %v", err)
	}
	setrace.InfoLogEvent(ctx, s.logger, "Session loaded", log.Any("session", makeSessionViewForLog(session)))
	ctx = context.WithValue(ctx, sessionKey{}, &session)
	defer func() {
		s.saveSession(ctx, session)
	}()
	return next(ctx, bot, eventType, event)
}

func NewSessionInterceptor(db db.DB, logger log.Logger, serviceLogger log.Logger) *SessionInterceptor {
	// todo: validate
	return &SessionInterceptor{
		db:            db,
		logger:        logger,
		serviceLogger: serviceLogger,
	}
}

type sessionKey struct{}

func (s *SessionInterceptor) loadSession(ctx context.Context, sessionID string) (model.Session, error) {
	ctx, cancel := context.WithTimeout(ctx, sessionLoadTimeout)
	defer cancel()
	session, err := s.db.Load(ctx, sessionID)
	if err != nil {
		if errors.Is(err, &model.SessionNotFoundError{}) {
			setrace.InfoLogEvent(ctx, s.logger, "No session found, creating new one")
			return s.updateNewSession(ctx, session, sessionID), nil
		}
		return model.Session{}, err
	}
	if session.LastUpdateTime.IsZero() {
		setrace.InfoLogEvent(ctx, s.logger, "No session found, creating new one")
		return s.updateNewSession(ctx, session, sessionID), nil
	}
	return session, nil
}

func (s *SessionInterceptor) saveSession(ctx context.Context, session model.Session) {
	ctx, cancel := context.WithTimeout(ctx, sessionSaveTimeout)
	defer cancel()
	if err := s.db.Save(ctx, session); err != nil {
		msg := fmt.Sprintf("Session saving error: %v", err)
		setrace.ErrorLogEvent(ctx, s.serviceLogger, msg)
		ctxlog.Error(ctx, s.serviceLogger, msg)
	} else {
		setrace.InfoLogEvent(ctx, s.logger, "Session saved", log.Any("session", makeSessionViewForLog(session)))
	}
}

func makeSessionViewForLog(session model.Session) interface{} {
	bytes, _ := json.Marshal(session)
	view := map[string]interface{}{}
	_ = json.Unmarshal(bytes, &view)
	return view
}

func (s *SessionInterceptor) updateNewSession(ctx context.Context, session model.Session, sessionID string) model.Session {
	session.ID = sessionID
	session.UUID = uuid.Must(uuid.NewV4()).String()
	return session
}

func (s *SessionInterceptor) uploadSession(ctx context.Context) {
	session := s.GetSession(ctx)
	if session.ID != "" {
		s.saveSession(ctx, *session)
	}
}

func (s *SessionInterceptor) GetSession(ctx context.Context) *model.Session {
	session, ok := ctx.Value(sessionKey{}).(*model.Session)
	if ok {
		return session
	}
	return new(model.Session)
}

package proxy

import (
	"context"
	"net/http"
	"net/http/httptest"
	"regexp"
	"testing"

	"a.yandex-team.ru/alice/library/go/middleware"
	"a.yandex-team.ru/alice/library/go/userctx"
	"a.yandex-team.ru/alice/library/go/zaplogger"

	"a.yandex-team.ru/alice/iot/steelix/config"
	"a.yandex-team.ru/alice/library/go/blackbox"
	quasartvm "a.yandex-team.ru/alice/library/go/tvm"
	"github.com/stretchr/testify/assert"
)

func TestDirector(t *testing.T) {
	nopLogger := zaplogger.NewNop()
	tvmClient := &quasartvm.ClientMock{
		Logger: nopLogger,
		ServiceTickets: map[string]string{
			"a": "ok",
		},
	}

	t.Run("parse_fail", func(t *testing.T) {
		director, err := newDirector(nopLogger, tvmClient, Config{
			URL: "%0",
		})

		assert.Nil(t, director)
		assert.Error(t, err)
	})

	t.Run("auth_type", func(t *testing.T) {
		t.Run("auth_type=tvm", func(t *testing.T) {
			director, err := newDirector(nopLogger, tvmClient, Config{URL: "schm://destination/api", AuthType: AuthTypeTVM, TvmAlias: "a"})

			assert.NoError(t, err)
			if assert.NotNil(t, director) {
				req := httptest.NewRequest("GET", "http://steelix/v1/azaza", nil)
				req.Header.Set("Authorization", "delete me")
				req.Header.Set("Cookie", "delete me")
				req.Header.Set("Host", "override me")
				req.Header.Set("User-Agent", "override me")
				req.Header.Set("Unknown", "keep me")
				req.Header.Set(xYaUserTicket, "override me")
				req.Header.Set(xYaServiceTicket, "override me")
				req = req.WithContext(userctx.WithUser(req.Context(), userctx.User{Ticket: "context-ticket"}))

				director(req)

				assert.Empty(t, req.Header.Get("Authorization"))
				assert.Empty(t, req.Header.Get("Cookie"))
				assert.Equal(t, req.Header.Get("Host"), "destination")
				assert.Equal(t, req.Header.Get("User-Agent"), "steelix/1.0")
				assert.Equal(t, req.Header.Get("Unknown"), "keep me")
				assert.Equal(t, req.Header.Get(xYaServiceTicket), "ok")
				assert.Equal(t, req.Header.Get(xYaUserTicket), "context-ticket")
			}
		})

		t.Run("auth_type=headers_and_tvm", func(t *testing.T) {
			director, err := newDirector(nopLogger, tvmClient, Config{URL: "schm://destination/api", AuthType: AuthTypeHeadersAndTVM, TvmAlias: "a"})

			assert.NoError(t, err)
			if assert.NotNil(t, director) {
				req := httptest.NewRequest("GET", "http://steelix/v1/azaza", nil)
				req.Header.Set("Authorization", "OAuth Bearer token")
				req.Header.Set("Cookie", "badGuy=1;")
				req.Header.Set("Host", "override me")
				req.Header.Set("User-Agent", "override me")
				req.Header.Set("Unknown", "keep me")
				req.Header.Set(xYaServiceTicket, "override me")
				req.Header.Set(xYaUserTicket, "override me")
				req = req.WithContext(userctx.WithUser(req.Context(), userctx.User{Ticket: "context-ticket"}))

				director(req)

				assert.Equal(t, req.Header.Get("Authorization"), "OAuth Bearer token")
				assert.Equal(t, req.Header.Get("Cookie"), "badGuy=1;")
				assert.Equal(t, req.Header.Get("Host"), "destination")
				assert.Equal(t, req.Header.Get("User-Agent"), "steelix/1.0")
				assert.Equal(t, req.Header.Get("Unknown"), "keep me")
				assert.Equal(t, req.Header.Get(xYaServiceTicket), "ok")
				assert.Equal(t, req.Header.Get(xYaUserTicket), "context-ticket")
			}
		})

		t.Run("auth_type=headers", func(t *testing.T) {
			director, err := newDirector(nopLogger, tvmClient, Config{URL: "schm://destination/api", AuthType: AuthTypeHeaders})

			assert.NoError(t, err)
			if assert.NotNil(t, director) {
				req := httptest.NewRequest("GET", "http://steelix/v1/azaza", nil)
				req.Header.Set("Authorization", "OAuth Bearer token")
				req.Header.Set("Cookie", "badGuy=1;")
				req.Header.Set("Host", "override me")
				req.Header.Set("User-Agent", "override me")
				req.Header.Set("Unknown", "keep me")

				director(req)

				assert.Equal(t, req.Header.Get("Authorization"), "OAuth Bearer token")
				assert.Equal(t, req.Header.Get("Cookie"), "badGuy=1;")
				assert.Equal(t, req.Header.Get("Host"), "destination")
				assert.Equal(t, req.Header.Get("User-Agent"), "steelix/1.0")
				assert.Equal(t, req.Header.Get("Unknown"), "keep me")
				assert.Empty(t, req.Header.Get(xYaUserTicket))
				assert.Empty(t, req.Header.Get(xYaServiceTicket))
			}
		})
	})

	t.Run("properties", func(t *testing.T) {
		director, err := newDirector(nopLogger, tvmClient, Config{URL: "schm://destination/api", AuthType: AuthTypeTVM, TvmAlias: "a"})

		assert.NoError(t, err)
		if assert.NotNil(t, director) {
			req := httptest.NewRequest("GET", "http://steelix/v1/azaza", nil)
			director(req)

			assert.Equal(t, "GET", req.Method)
			assert.Equal(t, "destination", req.Host)
			if assert.NotNil(t, req.URL) {
				assert.Equal(t, "schm", req.URL.Scheme)
				assert.Equal(t, "destination", req.URL.Host)
				assert.Equal(t, "/api/v1/azaza", req.URL.Path)
				assert.Equal(t, "", req.URL.RawQuery)
			}
		}
	})

	t.Run("query join", func(t *testing.T) {
		t.Run("empty", func(t *testing.T) {
			director, err := newDirector(nopLogger, tvmClient, Config{URL: "schm://destination/api", AuthType: AuthTypeTVM, TvmAlias: "a"})

			assert.NoError(t, err)
			if assert.NotNil(t, director) {
				req := httptest.NewRequest("GET", "http://steelix/v1/azaza", nil)
				director(req)

				assert.Equal(t, "", req.URL.RawQuery)
			}
		})

		t.Run("request", func(t *testing.T) {
			director, err := newDirector(nopLogger, tvmClient, Config{URL: "schm://destination/api", AuthType: AuthTypeTVM, TvmAlias: "a"})

			assert.NoError(t, err)
			if assert.NotNil(t, director) {
				req := httptest.NewRequest("GET", "http://steelix/v1/azaza?foo=bar", nil)
				director(req)

				assert.Equal(t, "foo=bar", req.URL.RawQuery)
			}
		})

		t.Run("target", func(t *testing.T) {
			director, err := newDirector(nopLogger, tvmClient, Config{URL: "schm://destination/api?bar=baz", AuthType: AuthTypeTVM, TvmAlias: "a"})

			assert.NoError(t, err)
			if assert.NotNil(t, director) {
				req := httptest.NewRequest("GET", "http://steelix/v1/azaza", nil)
				director(req)

				assert.Equal(t, "bar=baz", req.URL.RawQuery)
			}
		})

		t.Run("both", func(t *testing.T) {
			director, err := newDirector(nopLogger, tvmClient, Config{URL: "schm://destination/api?bar=baz&baz=fox", AuthType: AuthTypeTVM, TvmAlias: "a"})

			assert.NoError(t, err)
			if assert.NotNil(t, director) {
				req := httptest.NewRequest("GET", "http://steelix/v1/azaza?foo=bar&baz=fox", nil)
				director(req)

				assert.Equal(t, "bar=baz&baz=fox&foo=bar&baz=fox", req.URL.RawQuery)
			}
		})
	})

	t.Run("rewrites", func(t *testing.T) {
		director, err := newDirector(nopLogger, tvmClient, Config{
			URL: "schm://destination/api",
			Rewrites: []config.Rewrite{
				{From: "^/v1(/.*)", To: "/v1.0$1"},
			},
			AuthType: AuthTypeTVM, TvmAlias: "a",
		})

		assert.NoError(t, err)
		if assert.NotNil(t, director) {
			req := httptest.NewRequest("GET", "http://steelix/v1/azaza", nil)
			director(req)

			if assert.NotNil(t, req.URL) {
				assert.Equal(t, "/api/v1.0/azaza", req.URL.Path)
			}
		}
	})

	t.Run("tvm", func(t *testing.T) {
		t.Run("normal", func(t *testing.T) {
			director, err := newDirector(nopLogger, tvmClient, Config{
				URL:      "schm://destination/api",
				AuthType: AuthTypeTVM,
				TvmAlias: "a",
			})

			assert.NoError(t, err)
			if assert.NotNil(t, director) {
				req := httptest.NewRequest("GET", "http://steelix/v1/azaza", nil)
				director(req)

				assert.Equal(t, req.Header.Get(xYaServiceTicket), "ok")
			}
		})
		t.Run("panic", func(t *testing.T) {
			director, err := newDirector(nopLogger, tvmClient, Config{
				URL:      "schm://destination/api",
				AuthType: AuthTypeTVM,
				TvmAlias: "unknown",
			})

			assert.NoError(t, err)
			if assert.NotNil(t, director) {
				var err error
				func() { // catch panic helper
					defer func() {
						if e, ok := recover().(error); ok {
							err = e
						}
					}()

					req := httptest.NewRequest("GET", "http://steelix/v1/azaza", nil)
					director(req)
				}()

				assert.Error(t, err)
			}
		})
	})

	t.Run("user", func(t *testing.T) {
		director, err := newDirector(nopLogger, tvmClient, Config{
			URL:      "schm://destination/api",
			AuthType: AuthTypeTVM,
			TvmAlias: "a",
		})

		assert.NoError(t, err)
		if assert.NotNil(t, director) {
			bbClient := &blackbox.ClientMock{
				Logger:     nopLogger,
				User:       &userctx.User{ID: 1, Login: "me", Ticket: "super-secret-ticket"},
				OAuthToken: &userctx.OAuth{},
			}

			req := httptest.NewRequest("GET", "http://steelix/v1/azaza", nil)
			req.Header.Set("Authorization", "OAuth test_oauth_token")

			next := http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
				// save req.WithContext
				req = r
			})
			middleware.MultiAuthMiddleware(nopLogger,
				middleware.NewBlackboxOAuthUserExtractor(nopLogger, bbClient),
			)(next).ServeHTTP(httptest.NewRecorder(), req)

			director(req)

			assert.Equal(t, "super-secret-ticket", req.Header.Get(xYaUserTicket))
		}
	})
}

func TestBuildRewrites(t *testing.T) {
	type testCase struct {
		config Config
		result []rewriteRule
		err    interface{}
	}
	check := func(tc testCase) func(*testing.T) {
		return func(t *testing.T) {
			defer func() {
				err := recover()
				assert.Equal(t, tc.err, err)
			}()

			actual := buildRewrites(tc.config)

			assert.Equal(t, tc.result, actual)
		}
	}

	t.Run("empty", check(testCase{
		config: Config{
			Rewrites: nil,
		},
		result: nil,
		err:    nil,
	}))

	t.Run("normal", check(testCase{
		config: Config{
			Rewrites: []config.Rewrite{
				{From: ".*", To: "a"},
				{From: "^/api/.*", To: "ab$1"},
			},
		},
		result: []rewriteRule{
			{
				Regexp: regexp.MustCompile(".*"),
				To:     "a",
			},
			{
				Regexp: regexp.MustCompile("^/api/.*"),
				To:     "ab$1",
			},
		},
		err: nil,
	}))

	t.Run("panic", check(testCase{
		config: Config{
			Rewrites: []config.Rewrite{
				{From: ".*", To: "a"},
				{From: "$[^/api/.*", To: "ab$1"},
				{From: "^/api/.*", To: "ab$1"},
			},
		},
		result: nil,
		err:    "regexp: Compile(`$[^/api/.*`): error parsing regexp: missing closing ]: `[^/api/.*`",
	}))
}

func TestGetPath(t *testing.T) {
	type testCase struct {
		rewrites []rewriteRule
		basePath string
		reqPath  string
		result   string
	}
	check := func(tc testCase) func(*testing.T) {
		return func(t *testing.T) {
			nopLogger := zaplogger.NewNop()
			actual := getPath(context.Background(), nopLogger, tc.rewrites, tc.basePath, tc.reqPath)

			assert.Equal(t, tc.result, actual)
		}
	}

	t.Run("join", func(t *testing.T) {
		t.Run("no_slashes", check(testCase{
			basePath: "/left",
			reqPath:  "right",
			result:   "/left/right",
		}))
		t.Run("left_slash", check(testCase{
			basePath: "/left/",
			reqPath:  "right",
			result:   "/left/right",
		}))
		t.Run("right_slash", check(testCase{
			basePath: "/left",
			reqPath:  "/right",
			result:   "/left/right",
		}))
		t.Run("both_slashes", check(testCase{
			basePath: "/left/",
			reqPath:  "/right",
			result:   "/left/right",
		}))
	})

	t.Run("rewrite", func(t *testing.T) {
		t.Run("remove_prefix", func(t *testing.T) {
			rewrites := []rewriteRule{
				{
					Regexp: regexp.MustCompile("^/prefix(/.*)"),
					To:     "$1",
				},
			}

			t.Run("no_prefix", check(testCase{
				rewrites: rewrites,
				basePath: "/",
				reqPath:  "/api",
				result:   "/api",
			}))
			t.Run("normal", check(testCase{
				rewrites: rewrites,
				basePath: "/internal",
				reqPath:  "/prefix/api",
				result:   "/internal/api",
			}))
		})
		t.Run("replace_all", check(testCase{
			rewrites: []rewriteRule{
				{
					Regexp: regexp.MustCompile("a"),
					To:     "A",
				},
			},
			basePath: "/base",
			reqPath:  "/api/internal",
			result:   "/base/Api/internAl",
		}))
		t.Run("chain", func(t *testing.T) {
			rewrites := []rewriteRule{
				{
					Regexp: regexp.MustCompile("^/api/v2(/.*)?"),
					To:     "/api/v2.0$1",
				},
				{
					Regexp: regexp.MustCompile("^/prefix(/.*)?"),
					To:     "$1",
				},
				{
					Regexp: regexp.MustCompile("^/api/v1(/.*)?"),
					To:     "/api/v1.0$1",
				},
			}

			t.Run("no_match", check(testCase{
				rewrites: rewrites,
				basePath: "/base",
				reqPath:  "/api/v3/endpoint",
				result:   "/base/api/v3/endpoint",
			}))
			t.Run("order", check(testCase{
				rewrites: rewrites,
				basePath: "/base",
				reqPath:  "/prefix/api/v2/endpoint",
				result:   "/base/api/v2/endpoint",
			}))
			t.Run("order2", check(testCase{
				rewrites: rewrites,
				basePath: "/base",
				reqPath:  "/prefix/api/v1/endpoint",
				result:   "/base/api/v1.0/endpoint",
			}))
			t.Run("optional", check(testCase{
				rewrites: rewrites,
				basePath: "/base",
				reqPath:  "/api/v1",
				result:   "/base/api/v1.0",
			}))
		})
	})
}

#include "parser1x.h"
#include "skill.h"

#include <alice/bass/forms/context/context.h>

#include <alice/bass/ut/helpers.h>
#include <alice/library/unittest/fake_fetcher.h>

#include <library/cpp/http/coro/server.h>
#include <library/cpp/scheme/scheme.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/tests_data.h>
#include <library/cpp/threading/future/async.h>

#include <util/charset/wide.h>
#include <util/generic/strbuf.h>
#include <util/thread/pool.h>
#include <util/thread/factory.h>


using namespace NBASS;
using namespace NBASS::NExternalSkill;

namespace {

constexpr TStringBuf REQUEST_FOR_SESSION_JSON = TStringBuf(R"-(
{
  "meta": {
    "epoch": 1504271099,
    "tz": "UTC",
    "uuid": "00000000-0000-0000-0000-000000000000",
    "utterance": "hello",
    "uid": 4007095345,
    "client_id" : "ru.yandex.searchplugin.dev/7.10 (none none; android 7.1.2)",
    "user_agent" : "Mozilla/5.0 (Linux; Android 7.1.2; Redmi Note 4 Build/NJH47F; wv) AppleWebKit/537.36 (KHTML, like Gecko) Version/4.0 Chrome/63.0.3239.111 Mobile Safari/537.36 YandexSearch/7.10",
  },
  "form": {
    "slots": [
      {
        "name": "skill_id",
        "type": "skill",
        "optional": false,
        "value": "5e5a25a4-3bbd-4e41-9c91-d893fe90bb82"
      },
      {
        "name": "skill_description",
        "type": "skill",
        "optional": false,
        "value": {
          "id": "5e5a25a4-3bbd-4e41-9c91-d893fe90bb82",
          "salt": "059bc345-fab3-44c9-bbc7-c3c56c81a9a1",
          "onAir": true,
          "logo": {
              "avatarId": "abc/def"
          },
          "storeUrl": "http://yandex.ru",
          "backendSettings": {
              "uri": "http://dale.search.yandex.net:13204/game"
          }
        }
      },
      {
        "name": "request",
        "type": "string",
        "optional": true,
        "value": "hello"
      }
    ],
    "name": "personal_assistant.scenarios.external_skill"
  }
}
)-");

constexpr TStringBuf REQUEST_FOR_PARSER_JSON = TStringBuf(R"-(
{
  "meta": {
    "epoch": 1504271099,
    "tz": "UTC",
    "uuid": "00000000-0000-0000-0000-000000000000",
    "utterance": "hello",
    "uid": 4007095345,
    "client_id" : "ru.yandex.searchplugin.dev/7.10 (none none; android 7.1.2)",
    "user_agent" : "Mozilla/5.0 (Linux; Android 7.1.2; Redmi Note 4 Build/NJH47F; wv) AppleWebKit/537.36 (KHTML, like Gecko) Version/4.0 Chrome/63.0.3239.111 Mobile Safari/537.36 YandexSearch/7.10",
  },
  "form": {
    "slots": [
      {
        "name": "skill_id",
        "type": "skill",
        "optional": false,
        "value": "5e5a25a4-3bbd-4e41-9c91-d893fe90bb82"
      },
      {
        "name": "skill_description",
        "type": "skill",
        "optional": false,
        "value": {
          "id": "5e5a25a4-3bbd-4e41-9c91-d893fe90bb82",
          "salt": "059bc345-fab3-44c9-bbc7-c3c56c81a9a1",
          "onAir": true,
          "name": "Игрушка финдера",
          "backendSettings": {
              "uri": "http://dale.search.yandex.net:13204/game"
          },
          "logo": {
              "avatarId": "abc/def"
          },
          "storeUrl": "http://yandex.ru",
          "backendSettings": {
              "uri": "http://dale.search.yandex.net:13204/game"
          }
        }
      },
      {
        "name": "request",
        "type": "string",
        "optional": true,
        "value": "hello"
      }
    ],
    "name": "personal_assistant.scenarios.external_skill"
  }
}
)-");

constexpr TStringBuf REQUEST_FOR_SKILL = TStringBuf(R"-(
{
  "meta": {
    "epoch": 1504271099,
    "tz": "UTC",
    "uuid": "00000000-0000-0000-0000-000000000000",
    "utterance": "hello",
    "uid": 4007095345,
    "client_id" : "ru.yandex.searchplugin.dev/7.10 (none none; android 7.1.2)",
    "user_agent" : "Mozilla/5.0 (Linux; Android 7.1.2; Redmi Note 4 Build/NJH47F; wv) AppleWebKit/537.36 (KHTML, like Gecko) Version/4.0 Chrome/63.0.3239.111 Mobile Safari/537.36 YandexSearch/7.10",
  },
  "form": {
    "slots": [
      {
        "name": "skill_id",
        "type": "skill",
        "optional": false,
        "value": "42"
      },
      {
        "name": "request",
        "type": "string",
        "optional": true,
        "value": "hello"
      }
    ],
    "name": "personal_assistant.scenarios.external_skill"
  }
}
)-");

const TString FAKE_RESPONSE_FOR_PARSER = R"-({"nlu":{"entities":[],"tokens":["hello"]}})-";
#if 0
constexpr TStringBuf SKILL_RESPONSE_VERSION_1_1 = TStringBuf(R"(
{"version":"1.0","response":{"text":"Вы можете осмотреть стол, тренажёр или фотографии внимательнее, подняться по лестнице, или вернуться в коридор.","end_session":false},"session":{"session_id":"","message_id":1,"user_id":"гыыуукк"}}
)");
#endif

class TSkills {
public:
    TSkills(std::unique_ptr<ISkillResolver> resolver = {}) {
        ISkillResolver::ResetGlobalResolver(std::move(resolver));
    }
};

// TODO this is not used, implement it!!!
class TSkillsServer: public IThreadFactory::IThreadAble {
private:
    TSkillsServer()
        : Port(TPortManager().GetTcpPort())
    {
        Thread = SystemThreadFactory()->Run(this);
    }

    ~TSkillsServer() override {
        Server.ShutDown();
        Thread->Join();
    }

    void DoExecute() override {
        NCoroHttp::THttpServer::TConfig cfg;
        cfg.SetPort(Port);
        NCoroHttp::THttpServer::TCallbacks cbs;
        cbs.SetRequestCb([this](NCoroHttp::THttpServer::TRequestContext& ctx) { OnResponse(ctx); } );
        Server.RunCycle(cfg, cbs);
    }

private:
    void OnResponse(NCoroHttp::THttpServer::TRequestContext& ctx) {
        auto* input = ctx.Input;
        Cerr << input->FirstLine() << Endl;
        // TODO implement it
    }

private:
    static const TSkillsServer Instance;
    NCoroHttp::THttpServer Server;
    THolder<IThreadFactory::IThread> Thread;

public:
    const ui32 Port;
};

const TSkillsServer TSkillsServer::Instance;

}

Y_UNIT_TEST_SUITE_F(ExternalSkill, NTestingHelpers::TBassContextFixture) {
    Y_UNIT_TEST(Session) {
        TContext::TPtr ctx = MakeContext(REQUEST_FOR_SESSION_JSON);
        UNIT_ASSERT(ctx);

        TString firstSessionId;
        { // check for new session
            TSession session(*ctx);

            UNIT_ASSERT(session.IsNew());
            UNIT_ASSERT_VALUES_EQUAL(session.SeqNum(), 0);
            UNIT_ASSERT(session.Id().size() > 0);
            firstSessionId = session.Id();

            session.UpdateContext(*ctx);
            const TContext::TSlot* slot = ctx->GetSlot("session");
            UNIT_ASSERT(!IsSlotEmpty(slot));
            UNIT_ASSERT(slot->Value["id"].GetString().size() > 0);
            UNIT_ASSERT_VALUES_EQUAL(slot->Value["seq"].GetIntNumber(-1), 0);
        }
        { // check session validity if session slot has already created
            TSession session(*ctx);

            UNIT_ASSERT(!session.IsNew());
            UNIT_ASSERT_VALUES_EQUAL(session.SeqNum(), 1);
            UNIT_ASSERT_VALUES_EQUAL(session.Id(), firstSessionId);

            session.UpdateContext(*ctx);
            const TContext::TSlot* slot = ctx->GetSlot("session");
            UNIT_ASSERT(!IsSlotEmpty(slot));
            UNIT_ASSERT_VALUES_EQUAL(slot->Value["id"].GetString(), firstSessionId);
            UNIT_ASSERT_VALUES_EQUAL(slot->Value["seq"].GetIntNumber(-1), 1);
        }
    }

    Y_UNIT_TEST(SkillDescription) {
        TSkills skills;
        // FIXME add invalid skills
        TContext::TPtr ctx = MakeContext(REQUEST_FOR_PARSER_JSON);
        UNIT_ASSERT(ctx);
        const TContext::TSlot* slot = ctx->GetSlot("skill_description");
        UNIT_ASSERT(!IsSlotEmpty(slot));
        const TSkillDescription skillDescription(*slot, *ctx, new NAlice::NTestingHelpers::TFakeRequest(FAKE_RESPONSE_FOR_PARSER));
        UNIT_ASSERT(skillDescription);
        UNIT_ASSERT(!skillDescription.Result());
        //TResultValue rval = skillDescription.Re
    }

#if 0
    Y_UNIT_TEST(Version1x) {
        TContext::TPtr ctx = MakeContext(REQUEST_FOR_PARSER_JSON);
        UNIT_ASSERT(ctx);
        TSession session(*ctx);

        TSkillParserVersion1x parser(NSc::TValue::FromJson(SKILL_RESPONSE_VERSION_1_1));
    }
#endif

    Y_UNIT_TEST(SkillValidation1x) {
        TSkills skills;
        TContext::TPtr ctx = MakeContext(REQUEST_FOR_PARSER_JSON);
        UNIT_ASSERT(ctx);
        const TContext::TSlot* slot = ctx->GetSlot("skill_description");
        UNIT_ASSERT(!IsSlotEmpty(slot));
        const TSkillDescription skillDescription(*slot, *ctx, new NAlice::NTestingHelpers::TFakeRequest(FAKE_RESPONSE_FOR_PARSER));
        UNIT_ASSERT(skillDescription);
        UNIT_ASSERT(!skillDescription.Result());

        TSession session(*ctx);

        // TODO needs to be improved
        const NSc::TValue respBigImage = NSc::TValue::FromJson(R"(
        {
            "response" : {
                "buttons" : [
                    {
                        "title" : "b1"
                    }
                ],
                "card" : {
                    "button" : {
                        "url" : "https://kjsksj"
                    },
                    "description" : "Описание",
                    "image_id" : "3880/0c27251bffefb0c37cad",
                    "title" : "Заголовок",
                    "type" : "BigImage"
                },
                "end_session" : false,
                "text" : "Текст",
                "tts" : "Текст TTS"
            },
            "session" : {
                "message_id" : 3,
                "session_id" : "cde7514-c0329f13-371c7bc-5",
                "user_id" : "BA98965DDBA3ADB06F3A092C33BD0D36A31D8D1BCB4247DB072352AD8DDC9301"
            },
            "version" : "1.0"
        }
        )");

        auto checkCb = [&ctx, &respBigImage, &skillDescription, &session](std::function<void(NSc::TValue&)> m, std::initializer_list<std::pair<TStringBuf, TStringBuf>> l) {
            NSc::TValue r = respBigImage.Clone();
            TContext* respCtx = nullptr;
            m(r);
            TSkillParserVersion1x sparser(session, skillDescription, r);
            auto err = sparser.CreateVinsAnswer(*ctx, &respCtx, [](TContext&) {});
            UNIT_ASSERT(respCtx);
            UNIT_ASSERT(err.Defined());
            for (const auto& i : l) {
                TString e = err->Data.TrySelect(i.first).ForceString();
                UNIT_ASSERT_EQUAL_C(e, i.second, TStringBuilder() << ">>> " << i.second.data() << ": " << e);
            }
        };

        checkCb([](auto& d) { d["response"]["card"]["button"].Delete("url"); }, { { "problems/0/type", "api" } });
        checkCb([](auto& d) { d["response"]["card"]["button"]["url"].SetString("hhh://skjskjs"); }, { { "problems/0/path", "button/url" }, { "problems/0/type", "bad_url" } });
        checkCb([](auto& d) { d["response"]["card"]["title"].SetString(WideToUTF8(TUtf16String(129, L'Ю'))); }, { { "problems/0/path", "/response/card/title" }, { "problems/0/type", "size_exceeded" } });
        checkCb([](auto& d) { d["response"]["card"]["description"].SetString(WideToUTF8(TUtf16String(257, L'Ю'))); }, { { "problems/0/path", "/response/card/description" }, { "problems/0/type", "size_exceeded" } });
        checkCb([](auto& d) { d["response"]["card"]["image_id"].SetString(""); }, { { "problems/0/path", "/response/card/image_id" } });
        checkCb([](auto& d) { d["response"]["card"]["image_id"].SetString("abc*"); }, { { "problems/0/path", "/response/card/image_id" } });
        checkCb([](auto& d) { d["response"]["card"]["type"].SetString("BadType*"); }, { { "problems/0/path", "/response/card/type" }, { "problems/0/type", "invalid_card_type" } });
    }

    Y_UNIT_TEST(TestSkillResolver) {
        bool resolved = false;

        class TMockSkillResolver : public ISkillResolver {
        public:
            TMockSkillResolver(bool* resolved)
                    : Resolved(resolved) {
            }

            TSkillResponsePtr ResolveSkillId(TContext&, TStringBuf skillId, const TConfig&,
                                             TErrorBlock::TResult* error) const override {
                if (skillId == "42") {
                    NSc::TValue value;
                    value["result"]["id"] = 42;
                    value["result"]["salt"] = "059bc345-fab3-44c9-bbc7-c3c56c81a9a1";
                    value["result"]["onAir"] = "true";
                    value["result"]["logo"]["avatarId"] = "abc/def";
                    value["result"]["storeUrl"] = "http://yandex.ru";
                    value["result"]["backendSettings"]["uri"] = "http://dale.search.yandex.net:13204/game";
                    *Resolved = true;
                    return THolder(new TApiSkillResponse(value));
                } else {
                    if (error)
                        error->ConstructInPlace(TError::EType::SKILLSERROR, "unknown");
                    return nullptr;
                }
            }
            TVector<TSkillResponsePtr> ResolveSkillIds(TContext&, const TVector<TStringBuf>&, const TConfig&,
                                                       TErrorBlock::TResult*) const override {
                return {};
            }

        private:
            bool* Resolved;
        };

        TSkills skills{std::make_unique<TMockSkillResolver>(&resolved)};
        TContext::TPtr ctx = MakeContext(REQUEST_FOR_SKILL);
        UNIT_ASSERT(ctx);

        const TContext::TSlot* slot = ctx->GetSlot("skill_id");
        UNIT_ASSERT(!IsSlotEmpty(slot));

        TSkillDescription skillDescription(*ctx);
        skillDescription.Init(*slot, *ctx);

        UNIT_ASSERT(skillDescription);
        UNIT_ASSERT(resolved);
    }
}

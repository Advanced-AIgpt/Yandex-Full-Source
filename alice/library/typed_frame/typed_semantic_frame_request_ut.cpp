#include "typed_semantic_frame_request.h"

#include <alice/megamind/protos/common/frame.pb.h>
#include <alice/megamind/protos/common/iot.pb.h>
#include <alice/protos/data/contacts.pb.h>
#include <alice/protos/data/news_provider.pb.h>
#include <alice/protos/data/scenario/order/order.pb.h>
#include <alice/protos/data/scenario/alice_show/selectors.pb.h>
#include <alice/protos/data/scenario/music/content_id.pb.h>
#include <alice/protos/data/scenario/music/topic.pb.h>
#include <alice/protos/data/scenario/video_call/video_call.pb.h>
#include <alice/protos/data/tv_feature_boarding/template.pb.h>
#include <alice/protos/endpoint/capability.pb.h>
#include <alice/protos/endpoint/capabilities/opening_sensor/capability.pb.h>
#include <alice/protos/endpoint/endpoint.pb.h>
#include <alice/protos/endpoint/events/events.pb.h>

#include <alice/library/json/json.h>
#include <alice/library/unittest/message_diff.h>

#include <google/protobuf/struct.pb.h>
#include <google/protobuf/wrappers.pb.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/string.h>

namespace {

using namespace NAlice;

TTypedSemanticFrameRequest FromTextJson(TStringBuf jsonString) {
    const auto json = JsonFromString(jsonString);
    return TTypedSemanticFrameRequest{JsonToProto<google::protobuf::Struct>(json)};
}

TSemanticFrame FrameFromTextJson(TStringBuf jsonString) {
    return FromTextJson(jsonString).SemanticFrame;
}

TMaybe<TOrigin> OriginFromTextJson(TStringBuf jsonString) {
    return FromTextJson(jsonString).Origin;
}

template <typename T>
void AddSlot(const TString& name, const TString& type, const T& value, const TString& acceptType, TSemanticFrame& frame) {
    auto& slot = *frame.AddSlots();
    slot.SetName(name);
    slot.SetType(type);
    if constexpr (std::is_base_of_v<NProtoBuf::Message, T>) {
        slot.SetValue(JsonStringFromProto(value));
    } else {
        slot.SetValue(ToString(value));
    }
    slot.AddAcceptedTypes(acceptType);
}

template <typename T>
void AddSlot(const TString& name, const TString& type, const T& value, TSemanticFrame& frame) {
    AddSlot(name, type, value, type, frame);
}

NData::TNewsProvider MakeNewsProvider(const TString& source, const TString& rubric) {
    NData::TNewsProvider provider;
    provider.SetNewsSource(source);
    provider.SetRubric(rubric);
    return provider;
}

NData::NMusic::TTopic MakeTopic(const TString& podcast) {
    NData::NMusic::TTopic topic;
    topic.SetPodcast(podcast);
    return topic;
}

TIoTCapabilityAction MakeIoTTTSCapabilityAction(const TString& text) {
    TIoTUserInfo::TCapability::TQuasarCapabilityState::TTtsValue ttsValue;
    ttsValue.SetText(text);
    TIoTUserInfo::TCapability::TQuasarCapabilityState state;
    state.SetInstance("tts");
    state.MutableTtsValue()->CopyFrom(ttsValue);
    TIoTCapabilityAction capabilityAction;
    capabilityAction.SetType(TIoTUserInfo::TCapability::QuasarCapabilityType);
    capabilityAction.MutableQuasarCapabilityState()->CopyFrom(state);
    return capabilityAction;
}

TIoTCapabilityAction MakeIoTOnOffCapabilityAction(bool on) {
    TIoTUserInfo::TCapability::TOnOffCapabilityState state;
    state.SetInstance("on");
    state.SetValue(on);

    TIoTCapabilityAction capabilityAction;
    capabilityAction.SetType(TIoTUserInfo::TCapability::OnOffCapabilityType);
    capabilityAction.MutableOnOffCapabilityState()->CopyFrom(state);
    return capabilityAction;
}

TStartIotDiscoveryRequest MakeStartIotDiscoveryRequest() {
    TStartIotDiscoveryRequest request;
    request.AddProtocols(TIotDiscoveryCapability_TProtocol_Zigbee);
    return request;
}

TFinishIotDiscoveryRequest MakeFinishIotDiscoveryRequest() {
    TFinishIotDiscoveryRequest request;

    request.AddProtocols(TIotDiscoveryCapability_TProtocol_Zigbee);

    auto& endpoint = *request.MutableDiscoveredEndpoints()->Add();
    endpoint.SetId("example-lamp-id-1");

    auto& endpoint_meta = *endpoint.MutableMeta();
    endpoint_meta.SetType(TEndpoint_EEndpointType_LightEndpointType);

    TOnOffCapability on_off_capability;
    on_off_capability.MutableState()->SetOn(true);

    endpoint.MutableCapabilities()->Add()->PackFrom(on_off_capability);
    return request;
}

TForgetIotEndpointsRequest MakeForgetIotEndpointsRequest() {
    TForgetIotEndpointsRequest request;
    request.AddEndpointIds("lamp-device-id-1");
    return request;
}

TIoTYandexIOActionRequest MakeIoTYandexIOActionRequest() {
    TIoTYandexIOActionRequest request;
    auto& endpointAction = *request.MutableEndpointActions()->Add();
    endpointAction.SetExternalDeviceId("lamp-device-id-1");
    endpointAction.SetSkillId("YANDEX_IO");
    auto& capabilityAction = *endpointAction.MutableActions()->Add();
    capabilityAction.CopyFrom(MakeIoTOnOffCapabilityAction(true));
    return request;
}

TIoTDeviceActionsBatch MakeIoTDeviceActionsBatch() {
    TIoTDeviceActionsBatch batch;
    auto& endpointAction = *batch.MutableBatch()->Add();
    endpointAction.SetDeviceId("lamp-device-id-1");
    endpointAction.SetExternalDeviceId("lamp-device-ext-id-1");
    endpointAction.SetSkillId("YANDEX_IO");
    auto& capabilityAction = *endpointAction.MutableActions()->Add();
    capabilityAction.CopyFrom(MakeIoTOnOffCapabilityAction(true));
    return batch;
}

TEndpointStateUpdatesRequest MakeEndpointStateUpdatesRequest() {
    TEndpointStateUpdatesRequest request;
    auto& endpointUpdate = *request.MutableEndpointUpdates()->Add();
    endpointUpdate.SetId("example-socket-id-1");

    auto& endpoint_meta = *endpointUpdate.MutableMeta();
    endpoint_meta.SetType(TEndpoint_EEndpointType_SocketEndpointType);

    TOnOffCapability on_off_capability;
    on_off_capability.MutableState()->SetOn(true);

    endpointUpdate.MutableCapabilities()->Add()->PackFrom(on_off_capability);
    return request;
}

TEndpointEventsBatch MakeEndpointEventsBatch() {
    TEndpointEventsBatch batch;
    auto& endpointEvents = *batch.MutableBatch()->Add();
    endpointEvents.SetEndpointId("example-sensor-id-1");
    endpointEvents.MutableEndpointStatus()->SetStatus(TEndpoint_EEndpointStatus_Online);

    TOpeningSensorCapability_TOpeningSensorOpenedEvent openedEvent;
    auto& openedEventHolder = *endpointEvents.MutableCapabilityEvents()->Add();
    openedEventHolder.MutableEvent()->PackFrom(openedEvent);

    TOnOffCapability onOffCapability;
    onOffCapability.MutableState()->SetOn(true);
    TOnOffCapability_TUpdateStateEvent updateStateEvent;
    updateStateEvent.MutableCapability()->CopyFrom(onOffCapability);

    auto& updateStateEventHolder = *endpointEvents.MutableCapabilityEvents()->Add();
    updateStateEventHolder.MutableEvent()->PackFrom(updateStateEvent);
    return batch;
}

TSemanticFrame MakeIoTYandexIOActionSemanticFrame(const TIoTYandexIOActionRequest& request) {
    TSemanticFrame frame{};
    frame.SetName("alice.iot.yandex_io.action");

    AddSlot(/* name= */ "request", /* type= */ "custom.iot.yandex_io.action_request", /* value= */ request, /* acceptedType= */ "custom.iot.yandex_io.action_request", frame);

    auto& typedFrame = *frame.MutableTypedSemanticFrame()->MutableIotYandexIOActionSemanticFrame();
    typedFrame.MutableRequest()->MutableRequestValue()->CopyFrom(request);

    return frame;
}

TSemanticFrame MakeIoTScenarioStepActionsSemanticFrame(const TIoTDeviceActionsBatch& batch) {
    TSemanticFrame frame{};
    frame.SetName("alice.iot.scenario.step.actions");

    AddSlot("launch_id", "string", "my-launch-id", "string", frame);
    AddSlot("step_index", "uint32", 42, "uint32", frame);
    AddSlot(/* name= */ "device_actions_batch", /* type= */ "custom.iot.device_actions_batch", /* value= */ batch, /* acceptedType= */ "custom.iot.device_actions_batch", frame);

    auto& typedFrame = *frame.MutableTypedSemanticFrame()->MutableIotScenarioStepActionsSemanticFrame();
    typedFrame.MutableLaunchID()->SetStringValue("my-launch-id");
    typedFrame.MutableStepIndex()->SetUInt32Value(42);
    typedFrame.MutableDeviceActionsBatch()->MutableBatchValue()->CopyFrom(batch);

    return frame;
}

TSemanticFrame MakeEndpointStateUpdatesSemanticFrame(const TEndpointStateUpdatesRequest& request) {
    TSemanticFrame frame{};
    frame.SetName("alice.endpoint.state.updates");

    AddSlot(/* name= */ "request", /* type= */ "custom.endpoint.state.updates", /* value= */ request, /* acceptedType= */ "custom.endpoint.state.updates", frame);

    auto& typedFrame = *frame.MutableTypedSemanticFrame()->MutableEndpointStateUpdatesSemanticFrame();
    typedFrame.MutableRequest()->MutableRequestValue()->CopyFrom(request);

    return frame;
}

TSemanticFrame MakeEndpointEventsBatchSemanticFrame(const TEndpointEventsBatch& batch) {
    TSemanticFrame frame{};
    frame.SetName("alice.endpoint.events.batch");

    AddSlot(/* name= */ "batch", /* type= */ "custom.endpoint.events.batch", /* value= */ batch, /* acceptedType= */ "custom.endpoint.events.batch", frame);

    auto& typedFrame = *frame.MutableTypedSemanticFrame()->MutableEndpointEventsBatchSemanticFrame();
    typedFrame.MutableBatch()->MutableBatchValue()->CopyFrom(batch);

    return frame;
}

TSemanticFrame MakeSaveIotNetworksSemanticFrame() {
    TSemanticFrame frame{};
    frame.SetName("alice.iot.discovery.save_networks");

    TIotDiscoveryCapability_TNetworks networks;
    networks.MutableZigbeeNetwork()->set_value("impressive_payload");
    AddSlot(/* name= */ "networks", /* type= */ "custom.iot.networks", /* value= */ networks, /* acceptedType= */ "custom.iot.networks", frame);

    auto& typedFrame = *frame.MutableTypedSemanticFrame()->MutableSaveIotNetworksSemanticFrame();
    typedFrame.MutableNetworks()->MutableNetworksValue()->CopyFrom(networks);
    return frame;
}

TSemanticFrame MakeStartIotDiscoverySemanticFrame(const TStartIotDiscoveryRequest& request, const TString& sessionID) {
    TSemanticFrame frame{};
    frame.SetName("alice.iot.discovery.start");

    AddSlot("request", "custom.iot.discovery.start_request", request, "custom.iot.discovery.start_request", frame);
    AddSlot("session_id", "string", sessionID, "string", frame);

    auto& typed_frame = *frame.MutableTypedSemanticFrame()->MutableStartIotDiscoverySemanticFrame();
    typed_frame.MutableRequest()->MutableRequestValue()->CopyFrom(request);
    typed_frame.MutableSessionID()->SetStringValue(sessionID);

    return frame;
}

TSemanticFrame MakeFinishIotDiscoverySemanticFrame(const TFinishIotDiscoveryRequest& request) {
    TSemanticFrame frame{};
    frame.SetName("alice.iot.discovery.finish");

    AddSlot("request", "custom.iot.discovery.finish_request", request, "custom.iot.discovery.finish_request", frame);

    auto& typed_frame = *frame.MutableTypedSemanticFrame()->MutableFinishIotDiscoverySemanticFrame();
    typed_frame.MutableRequest()->MutableRequestValue()->CopyFrom(request);

    return frame;
}

TSemanticFrame MakeForgetIotEndpointsSemanticFrame(const TForgetIotEndpointsRequest& request) {
    TSemanticFrame frame{};
    frame.SetName("alice.iot.unlink.forget_endpoints");

    AddSlot("request", "custom.iot.unlink.forget_endpoints_request", request, "custom.iot.unlink.forget_endpoints_request", frame);

    auto& typed_frame = *frame.MutableTypedSemanticFrame()->MutableForgetIotEndpointsSemanticFrame();
    typed_frame.MutableRequest()->MutableRequestValue()->CopyFrom(request);

    return frame;
}

TSemanticFrame MakeReminderShootSemanticFrame(const TString& text, const TString& reminderId, time_t epoch, const TString& timeZone) {
    TSemanticFrame frame;
    frame.SetName("alice.reminders.on_shoot");

    AddSlot("id", "string", reminderId, "string", frame);
    AddSlot("text", "string", text, "string", frame);
    AddSlot("epoch", "epoch", epoch, "epoch", frame);
    AddSlot("timezone", "string", timeZone, "string", frame);

    auto& typedFrame = *frame.MutableTypedSemanticFrame()->MutableRemindersOnShootSemanticFrame();
    typedFrame.MutableText()->SetStringValue(text);
    typedFrame.MutableId()->SetStringValue(reminderId);
    typedFrame.MutableEpoch()->SetEpochValue(ToString(epoch));
    typedFrame.MutableTimeZone()->SetStringValue(timeZone);

    return frame;
}

TSemanticFrame MakeOrderNotificationSemanticFrame(const TString& provider_name, const TString& order_id) {
    TSemanticFrame frame;
    frame.SetName("alice.order.notification");

    AddSlot("provider_name", "string", provider_name, "string", frame);
    AddSlot("order_id", "string", order_id, "string", frame);
    AddSlot("order_notification_type", "order_notification_type_value", "EN_COURIER_IS_COMMING", "order_notification_type_value", frame);

    auto& typedFrame = *frame.MutableTypedSemanticFrame()->MutableOrderNotificationSemanticFrame();
    typedFrame.MutableProviderName()->SetStringValue(provider_name);
    typedFrame.MutableOrderId()->SetStringValue(order_id);
    typedFrame.MutableOrderNotificationType()->SetOrderNotificationTypeValue(NAlice::NOrder::ENotificationType::EN_COURIER_IS_COMMING);

    return frame;
}

TSemanticFrame MakeTvPromoTemplateRequestSemanticFrame(const bool IsTandemDeviceAvailable, const bool IsTandemConnected) {
    TSemanticFrame frame;
    frame.SetName("alice.tv.get_promo_template");

    TTvPromoTemplateRequestSemanticFrame& typedFrame = *frame.MutableTypedSemanticFrame()->MutableTvPromoTemplateRequestSemanticFrame();
    NAlice::NSmartTv::TTandemTemplate& tandemTemplate = *typedFrame.MutableChosenTemplate()->MutableTandemTemplate();
    tandemTemplate.SetIsTandemConnected(IsTandemConnected);
    tandemTemplate.SetIsTandemDevicesAvailable(IsTandemDeviceAvailable);

    AddSlot("chosen_template", "custom.smarttv.feature_boarding.tandem", JsonStringFromProto(tandemTemplate), "custom.smarttv.feature_boarding.tandem", frame);

    return frame;
}

TSemanticFrame MakeTvPromoTemplateShownReportSemanticFrame() {
    TSemanticFrame frame;
    frame.SetName("alice.tv.report_promo_template_shown");

    TTvPromoTemplateShownReportSemanticFrame& typedFrame = *frame.MutableTypedSemanticFrame()->MutableTvPromoTemplateShownReportSemanticFrame();

    NSmartTv::TTandemTemplate& tandemTemplate = *typedFrame.MutableChosenTemplate()->MutableTandemTemplate();

    AddSlot("chosen_template", "custom.smarttv.feature_boarding.tandem", JsonStringFromProto(tandemTemplate), "custom.smarttv.feature_boarding.tandem", frame);

    return frame;
}

TSemanticFrame MakeMusicOnboardingSemanticFrame() {
    TSemanticFrame frame;
    frame.SetName("alice.music_onboarding");
    frame.MutableTypedSemanticFrame()->MutableMusicOnboardingSemanticFrame();
    return frame;
}

TSemanticFrame MakeMusicOnboardingArtistsSemanticFrame() {
    TSemanticFrame frame;
    frame.SetName("alice.music_onboarding.artists");
    frame.MutableTypedSemanticFrame()->MutableMusicOnboardingArtistsSemanticFrame();
    return frame;
}

TSemanticFrame MakeMusicOnboardingGenresSemanticFrame() {
    TSemanticFrame frame;
    frame.SetName("alice.music_onboarding.genres");
    frame.MutableTypedSemanticFrame()->MutableMusicOnboardingGenresSemanticFrame();
    return frame;
}

TSemanticFrame MakeMusicOnboardingTracksSemanticFrame() {
    TSemanticFrame frame;
    frame.SetName("alice.music_onboarding.tracks");
    frame.MutableTypedSemanticFrame()->MutableMusicOnboardingTracksSemanticFrame();
    return frame;
}

TSemanticFrame MakeMusicOnboardingTracksReaskSemanticFrame(const TString& trackId) {
    TSemanticFrame frame;
    frame.SetName("alice.music_onboarding.tracks_reask");

    AddSlot("track_id", "string", trackId, "string", frame);

    auto& typedFrame = *frame.MutableTypedSemanticFrame()->MutableMusicOnboardingTracksReaskSemanticFrame();
    typedFrame.MutableTrackId()->SetStringValue(trackId);

    return frame;
}

Y_UNIT_TEST_SUITE(WalkerTypedSemanticFrames) {
    Y_UNIT_TEST(RemindersList) {
        auto createTsf = []() {
            TSemanticFrame frame;
            frame.SetName("alice.reminders.list");

            AddSlot("countdown", "uint32", 10, "uint32", frame);

            auto& typedFrame = *frame.MutableTypedSemanticFrame()->MutableRemindersListSemanticFrame();
            typedFrame.MutableCountdown()->SetUInt32Value(10);
            return frame;
        };
        UNIT_ASSERT_MESSAGES_EQUAL(
            FrameFromTextJson(R"({
                "typed_semantic_frame": {
                    "reminders_list_semantic_frame": {
                        "countdown": {
                            "uint32_value": 10
                        }
                    }
                },
                "analytics": {
                    "product_scenario": "Timetable",
                    "origin": "Scenario",
                    "purpose": "reminders_list"
                }
            })"),
            createTsf());
    }

    Y_UNIT_TEST(RemindersOnShoot) {
        UNIT_ASSERT_MESSAGES_EQUAL(
            FrameFromTextJson(R"({
                "typed_semantic_frame": {
                    "reminders_on_shoot_semantic_frame": {
                        "id": {
                            "string_value": "guid"
                        },
                        "text": {
                            "string_value": "remind me this"
                        },
                        "epoch": {
                            "epoch_value": "1234567"
                        },
                        "timezone": {
                            "string_value": "Europe/Moscow"
                        }
                    }
                },
                "analytics": {
                    "product_scenario": "Timetable",
                    "origin": "Scenario",
                    "purpose": "reminder_on_shoot"
                }
            })"),
            MakeReminderShootSemanticFrame("remind me this", "guid", 1234567, "Europe/Moscow"));
    }

    Y_UNIT_TEST(Origin) {
        TMaybe<TOrigin> origin = OriginFromTextJson(R"({
            "typed_semantic_frame": {
                "player_what_is_playing_semantic_frame": {
                }
            },
            "analytics": {
                "product_scenario": "music",
                "purpose": "multiroom_redirect",
                "origin": "Scenario"
            },
            "origin": {
                "device_id": "another-device-id",
                "uuid": "another-uuid"
            }
        })");

        UNIT_ASSERT(origin.Defined());

        TOrigin expectedOrigin;
        expectedOrigin.SetDeviceId("another-device-id");
        expectedOrigin.SetUuid("another-uuid");
        UNIT_ASSERT_MESSAGES_EQUAL(*origin, expectedOrigin);
    }

    Y_UNIT_TEST(AliceShow) {
        auto makeAliceShowSemanticFrame = [](const NData::TNewsProvider& newsProvider, const NData::NMusic::TTopic& topic,
                                             NData::NAliceShow::TDayPart::EValue dayPart, NData::NAliceShow::TAge::EValue age) {
            TSemanticFrame frame;
            frame.SetName("alice.alice_show.activate");

            AddSlot("news_provider", "news_provider", newsProvider, "news_provider", frame);
            AddSlot("topic", "topic", topic, "topic", frame);
            AddSlot("day_part", "custom.day_part", dayPart, "custom.day_part", frame);
            AddSlot("age", "custom.age", age, "custom.age", frame);

            auto& typedFrame = *frame.MutableTypedSemanticFrame()->MutableAliceShowActivateSemanticFrame();
            typedFrame.MutableNewsProvider()->MutableNewsProviderValue()->CopyFrom(newsProvider);
            typedFrame.MutableTopic()->MutableTopicValue()->CopyFrom(topic);
            typedFrame.MutableDayPart()->SetDayPartValue(dayPart);
            typedFrame.MutableAge()->SetAgeValue(age);

            return frame;
        };
        UNIT_ASSERT_MESSAGES_EQUAL(
            FrameFromTextJson(R"({
                "typed_semantic_frame": {
                    "alice_show_activate_semantic_frame": {
                        "news_provider": {
                            "news_provider_value": {
                                "news_source": "a-source",
                                "rubric": "a-rubric"
                            }
                        },
                        "topic": {
                            "topic_value": {
                                "podcast": "a-podcast"
                            }
                        },
                        "day_part": {
                            "day_part_value": "Evening"
                        },
                        "age": {
                            "age_value": "Children"
                        }
                    }
                },
                "analytics": {
                    "product_scenario": "AliceShow",
                    "origin": "Scenario",
                    "purpose": "alice_show"
                }
            })"),
            makeAliceShowSemanticFrame(MakeNewsProvider("a-source", "a-rubric"), MakeTopic("a-podcast"),
                                       NData::NAliceShow::TDayPart::Evening, NData::NAliceShow::TAge::Children));
    }

    Y_UNIT_TEST(AppsFixlist) {
        auto makeAppsFixlistSemanticFrame = [](const TString& app_data) {
            TSemanticFrame frame;
            frame.SetName("alice.apps_fixlist");

            AddSlot("app_data", "custom.app_data", app_data, "custom.app_data", frame);

            auto& typedFrame = *frame.MutableTypedSemanticFrame()->MutableAppsFixlistSemanticFrame();
            typedFrame.MutableAppData()->SetAppDataValue(app_data);

            return frame;
        };
        UNIT_ASSERT_MESSAGES_EQUAL(
            FrameFromTextJson(R"({
                "typed_semantic_frame": {
                    "apps_fixlist_semantic_frame": {
                        "app_data": {
                            "app_data_value": "123"
                        }
                    }
                },
                "analytics": {
                    "product_scenario": "AppsFixlist",
                    "origin": "Scenario",
                    "purpose": "apps_fixlist"
                }
            })"),
            makeAppsFixlistSemanticFrame("123"));
    }

    Y_UNIT_TEST(OrderNotificator) {
        UNIT_ASSERT_MESSAGES_EQUAL(
            FrameFromTextJson(R"({
                "typed_semantic_frame": {
                    "order_notification_semantic_frame": {
                        "provider_name": {
                            "string_value": "lavka"
                        },
                        "order_id":{
                            "string_value": "123456"
                        },
                        "order_notification_type": {
                            "order_notification_type_value": 1
                        }
                    }
                },
                "analytics": {
                    "origin": "Scenario",
                    "purpose": "get_order_status"
                }
            })"),
            MakeOrderNotificationSemanticFrame("lavka","123456"));
    }

    Y_UNIT_TEST(GetSmartTvCarousel) {
        auto makeSmartTvCarouselSemanticFrame = [](
                const TString& CarouselId, const TString& DocsCacheHah, const TString& CarouselType,
                const TString& Filter, const TString& Tag, const bool AvailableOnly, const int MoreUrlLimit,
                const int Limit, const int Offset, const bool KidMode, int RestrictionAge) {
            TSemanticFrame frame;
            frame.SetName("alice.smarttv.get_carousel");

            AddSlot("carousel_id", "string", CarouselId, "string", frame);
            AddSlot("docs_cache_hash", "string", DocsCacheHah, "string", frame);
            AddSlot("carousel_type", "string", CarouselType, "string", frame);
            AddSlot("filter", "string", Filter, "string", frame);
            AddSlot("tag", "string", Tag, "string", frame);
            AddSlot("available_only", "bool", AvailableOnly, "bool", frame);
            AddSlot("more_url_limit", "num", MoreUrlLimit, "num", frame);
            AddSlot("limit", "num", Limit, "num", frame);
            AddSlot("offset", "num", Offset, "num", frame);
            AddSlot("kid_mode", "bool", KidMode, "bool", frame);
            AddSlot("restriction_age", "num", RestrictionAge, "num", frame);

            auto& typedFrame = *frame.MutableTypedSemanticFrame()->MutableGetSmartTvCarouselSemanticFrame();

            typedFrame.MutableCarouselId()->SetStringValue(CarouselId);
            typedFrame.MutableDocCacheHash()->SetStringValue(DocsCacheHah);
            typedFrame.MutableCarouselType()->SetStringValue(CarouselType);
            typedFrame.MutableFilter()->SetStringValue(Filter);
            typedFrame.MutableTag()->SetStringValue(Tag);
            typedFrame.MutableAvailableOnly()->SetBoolValue(AvailableOnly);
            typedFrame.MutableMoreUrlLimit()->SetNumValue(MoreUrlLimit);
            typedFrame.MutableLimit()->SetNumValue(Limit);
            typedFrame.MutableOffset()->SetNumValue(Offset);
            typedFrame.MutableKidMode()->SetBoolValue(KidMode);
            typedFrame.MutableRestrictionAge()->SetNumValue(RestrictionAge);

            return frame;
        };

        const TStringBuf expectedTypedSemanticFrameRaw = R"({
            "typed_semantic_frame": {
                "get_smart_tv_carousel_semantic_frame": {
                    "carousel_id": {
                        "string_value": "testcarouselid"
                    },
                    "docs_cache_hash": {
                        "string_value": "testdocscachehash"
                    },
                    "carousel_type": {
                        "string_value": "external_kp"
                    },
                    "filter": {
                        "string_value": "key1=1|key2=abc"
                    },
                    "tag": {
                        "string_value": "movie"
                    },
                    "available_only": {
                        "bool_value": false
                    },
                    "more_url_limit": {
                        "num_value": 11
                    },
                    "limit": {
                        "num_value": 12
                    },
                    "offset": {
                        "num_value": 21
                    },
                    "kid_mode": {
                        "bool_value": true
                    },
                    "restriction_age": {
                        "num_value": 6
                    }
                }
            },
            "analytics": {
                "origin": "Scenario",
                "purpose": "get_smart_tv_carousel"
            }
        })";

        const auto &actualTypedSemanticFrame = makeSmartTvCarouselSemanticFrame(
                "testcarouselid",
                "testdocscachehash",
                "external_kp",
                "key1=1|key2=abc",
                "movie",
                false,
                11,
                12,
                21,
                true,
                6);

        UNIT_ASSERT_MESSAGES_EQUAL(FrameFromTextJson(expectedTypedSemanticFrameRaw), actualTypedSemanticFrame);
    }

    Y_UNIT_TEST(GetSmartTvCarousels) {
        auto makeSmartTvCarouselsSemanticFrame = [](
                const TString& CategoryId, const int MaxItemsCount, const TString& CacheHash, const int Limit,
                const int Offset, const bool KidMode, int RestrictionAge, const int ExternalCarouselOffset) {
            TSemanticFrame frame;
            frame.SetName("alice.smarttv.get_carousels");

            AddSlot("category_id", "string", CategoryId, "string", frame);
            AddSlot("max_items_count", "num", MaxItemsCount, "num", frame);
            AddSlot("cache_hash", "string", CacheHash, "string", frame);
            AddSlot("limit", "num", Limit, "num", frame);
            AddSlot("offset", "num", Offset, "num", frame);
            AddSlot("kid_mode", "bool", KidMode, "bool", frame);
            AddSlot("restriction_age", "num", RestrictionAge, "num", frame);
            AddSlot("external_carousel_offset", "num", ExternalCarouselOffset, "num", frame);

            auto& typedFrame = *frame.MutableTypedSemanticFrame()->MutableGetSmartTvCarouselsSemanticFrame();

            typedFrame.MutableCategoryId()->SetStringValue(CategoryId);
            typedFrame.MutableMaxItemsCount()->SetNumValue(MaxItemsCount);
            typedFrame.MutableCacheHash()->SetStringValue(CacheHash);
            typedFrame.MutableLimit()->SetNumValue(Limit);
            typedFrame.MutableOffset()->SetNumValue(Offset);
            typedFrame.MutableKidMode()->SetBoolValue(KidMode);
            typedFrame.MutableRestrictionAge()->SetNumValue(RestrictionAge);
            typedFrame.MutableExternalCarouselOffset()->SetNumValue(ExternalCarouselOffset);

            return frame;
        };

        const TStringBuf expectedTypedSemanticFrameRaw = R"({
            "typed_semantic_frame": {
                "get_smart_tv_carousels_semantic_frame": {
                    "category_id": {
                        "string_value": "testcategoryid"
                    },
                    "cache_hash": {
                        "string_value": "testcachehash"
                    },
                    "limit": {
                        "num_value": 10
                    },
                    "offset": {
                        "num_value": 1
                    },
                    "max_items_count": {
                        "num_value": 12
                    },
                    "kid_mode": {
                        "bool_value": true
                    },
                    "restriction_age": {
                        "num_value": 6
                    },
                    "external_carousel_offset": {
                        "num_value": 13
                    }
                }
            },
            "analytics": {
                "origin": "Scenario",
                "purpose": "get_smart_tv_carousels"
            }
        })";

        const auto &actualTypedSemanticFrame = makeSmartTvCarouselsSemanticFrame(
                "testcategoryid",
                12,
                "testcachehash",
                10,
                1,
                true,
                6,
                13);

        UNIT_ASSERT_MESSAGES_EQUAL(FrameFromTextJson(expectedTypedSemanticFrameRaw), actualTypedSemanticFrame);
    }

    Y_UNIT_TEST(TIoTScenarioSpeakerAction) {
        auto makeTIoTScenarioSpeakerActionSemanticFrame = [](const TString& launchID, const uint32_t& stepIndex, const TIoTCapabilityAction& capabilityAction) {
            TSemanticFrame frame;
            frame.SetName("alice.iot.scenario.speaker.action");

            AddSlot("launch_id", "string", launchID, "string", frame);
            AddSlot("step_index", "uint32", stepIndex, "uint32", frame);
            AddSlot("capability_action", "custom.iot.capability.action", capabilityAction, "custom.iot.capability.action", frame);

            auto& typedFrame = *frame.MutableTypedSemanticFrame()->MutableIoTScenarioSpeakerActionSemanticFrame();
            typedFrame.MutableLaunchID()->SetStringValue(launchID);
            typedFrame.MutableStepIndex()->SetUInt32Value(stepIndex);
            typedFrame.MutableCapabilityAction()->MutableCapabilityActionValue()->CopyFrom(capabilityAction);

            return frame;
        };
        UNIT_ASSERT_MESSAGES_EQUAL(
            FrameFromTextJson(R"({
                "typed_semantic_frame": {
                    "iot_scenario_speaker_action_semantic_frame": {
                        "launch_id": {
                            "string_value": "some-id"
                        },
                        "step_index": {
                            "uint32_value": 5
                        },
                        "capability_action": {
                            "capability_action_value": {
                                "type": "QuasarCapabilityType",
                                "quasar_capability_state": {
                                    "instance": "tts",
                                    "tts_value": {
                                        "text": "фразочка"
                                    }
                                }
                            }
                        }
                    }
                },
                "analytics": {
                    "product_scenario": "IoTScenarios",
                    "origin": "Scenario",
                    "purpose": "send_scenario_speaker_action"
                }
            })"),
            makeTIoTScenarioSpeakerActionSemanticFrame("some-id", 5, MakeIoTTTSCapabilityAction("фразочка")));
    }

    Y_UNIT_TEST(TIotYandexIOAction) {
        const auto eventPayload = JsonToProto<google::protobuf::Struct>(JsonFromString(TStringBuf(R"({
                "typed_semantic_frame": {
                    "iot_yandex_io_action_semantic_frame": {
                        "request": {
                            "request_value": {
                                "endpoint_actions": [
                                    {
                                        "external_device_id": "lamp-device-id-1",
                                        "skill_id": "YANDEX_IO",
                                        "actions": [
                                            {
                                                "type": "OnOffCapabilityType",
                                                "on_off_capability_state": {
                                                    "instance": "on",
                                                    "value": true
                                                }
                                            }
                                        ]
                                    }
                                ]
                            }
                        }
                    }
                },
                "analytics": {
                    "product_scenario": "IoTScenarios",
                    "origin": "Scenario",
                    "purpose": "iot_yandex_io_action"
                }
            })")));
        const auto frame = TTypedSemanticFrameRequest{eventPayload};

        const auto request = MakeIoTYandexIOActionRequest();
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeIoTYandexIOActionSemanticFrame(request));
    }

    Y_UNIT_TEST(TIotStepActions) {
        const auto eventPayload = JsonToProto<google::protobuf::Struct>(JsonFromString(TStringBuf(R"({
                "typed_semantic_frame": {
                    "iot_scenario_step_actions_semantic_frame": {
                        "launch_id": {
                            "string_value": "my-launch-id"
                        },
                        "step_index": {
                            "uint32_value": 42
                        },
                        "device_actions_batch": {
                            "batch_value": {
                                "batch": [
                                    {
                                        "device_id": "lamp-device-id-1",
                                        "external_device_id": "lamp-device-ext-id-1",
                                        "skill_id": "YANDEX_IO",
                                        "actions": [
                                            {
                                                "type": "OnOffCapabilityType",
                                                "on_off_capability_state": {
                                                    "instance": "on",
                                                    "value": true
                                                }
                                            }
                                        ]
                                    }
                                ]
                            }
                        }
                    }
                },
                "analytics": {
                    "product_scenario": "IoTScenarios",
                    "origin": "Scenario",
                    "purpose": "iot_scenario_step_actions"
                }
            })")));
        const auto frame = TTypedSemanticFrameRequest{eventPayload};

        const auto batch = MakeIoTDeviceActionsBatch();
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeIoTScenarioStepActionsSemanticFrame(batch));
    }

    Y_UNIT_TEST(TIoTDeviceAction) {
        auto makeTIoTDeviceActionSemanticFrame = [](const TIoTDeviceActionRequest& deviceActionRequest) {
            TSemanticFrame frame;
            frame.SetName("alice.iot.action.device");

            AddSlot("request", "custom.iot.action.device.request", deviceActionRequest, "custom.iot.action.device.request", frame);

            auto typedFrame = frame.MutableTypedSemanticFrame()->MutableIoTDeviceActionSemanticFrame();
            typedFrame->MutableRequest()->MutableRequestValue()->CopyFrom(deviceActionRequest);

            return frame;
        };

        auto makeDeviceActionRequest = []() {
            TIoTDeviceActionRequest request;

            auto intentParameters = request.MutableIntentParameters();
            intentParameters->SetCapabilityType("devices.capabilities.on_off");
            intentParameters->SetCapabilityInstance("on");
            intentParameters->MutableCapabilityValue()->SetBoolValue(true);

            request.AddRoomIDs("room-1");
            request.AddHouseholdIDs("household-1");
            request.AddHouseholdIDs("household-2");
            request.AddDeviceIDs("device-1");
            request.AddDeviceTypes("devices.types.light");
            request.SetAtTimestamp(1649710800);

            return request;
        };

        UNIT_ASSERT_MESSAGES_EQUAL(
            FrameFromTextJson(R"({
                "typed_semantic_frame": {
                    "iot_device_action_semantic_frame": {
                        "request": {
                            "request_value": {
                                "intent_parameters": {
                                    "capability_type": "devices.capabilities.on_off",
                                    "capability_instance": "on",
                                    "capability_value": {
                                        "bool_value": true
                                    }
                                },
                                "room_ids": ["room-1"],
                                "household_ids": ["household-1", "household-2"],
                                "group_ids": [],
                                "device_ids": ["device-1"],
                                "device_types": ["devices.types.light"],
                                "at_timestamp": 1649710800
                            }
                        }
                    }
                },
                "analytics": {
                    "product_scenario": "IoT",
                    "origin": "Scenario",
                    "purpose": "device_action"
                }
            })"),
            makeTIoTDeviceActionSemanticFrame(makeDeviceActionRequest()));
    }

    Y_UNIT_TEST(TEndpointStateUpdates) {
        const auto eventPayload = JsonToProto<google::protobuf::Struct>(JsonFromString(TStringBuf(R"({
                "typed_semantic_frame": {
                    "endpoint_state_updates_semantic_frame": {
                        "request": {
                            "request_value": {
                                "endpoint_updates": [{
                                    "id": "example-socket-id-1",
                                    "meta": {
                                        "type": "SocketEndpointType"
                                    },
                                    "capabilities": [{
                                        "@type": "type.googleapis.com/NAlice.TOnOffCapability",
                                        "state": {"on": true}
                                    }]
                                }]
                            }
                        }
                    }
                },
                "analytics": {
                    "product_scenario": "IoTScenarios",
                    "origin": "Scenario",
                    "purpose": "endpoint_state_updates"
                }
            })")));
        const auto frame = TTypedSemanticFrameRequest{eventPayload};

        const auto request = MakeEndpointStateUpdatesRequest();
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeEndpointStateUpdatesSemanticFrame(request));
    }

    Y_UNIT_TEST(TEndpointEventsBatch) {
        const auto eventPayload = JsonToProto<google::protobuf::Struct>(JsonFromString(TStringBuf(R"({
                "typed_semantic_frame": {
                    "endpoint_events_batch_semantic_frame": {
                        "batch": {
                            "batch_value": {
                                "batch": [{
                                    "endpoint_id": "example-sensor-id-1",
                                    "endpoint_status": {"status": "Online"},
                                    "capability_events": [
                                        {
                                            "event": {
                                                "@type": "type.googleapis.com/NAlice.TOpeningSensorCapability.TOpeningSensorOpenedEvent"
                                            }
                                        },
                                        {
                                            "event": {
                                                "@type": "type.googleapis.com/NAlice.TOnOffCapability.TUpdateStateEvent",
                                                "capability": {"state": {"on": true}}
                                            }
                                        }
                                    ]
                                }]
                            }
                        }
                    }
                },
                "analytics": {
                    "product_scenario": "IoTScenarios",
                    "origin": "Scenario",
                    "purpose": "endpoint_events_batch"
                }
            })")));
        const auto frame = TTypedSemanticFrameRequest{eventPayload};

        const auto batch = MakeEndpointEventsBatch();
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeEndpointEventsBatchSemanticFrame(batch));
    }

    Y_UNIT_TEST(TSaveIotNetworksTSF) {
        const auto eventPayload = JsonToProto<google::protobuf::Struct>(JsonFromString(TStringBuf(R"({
                "typed_semantic_frame": {
                    "save_iot_networks_semantic_frame": {
                        "networks": {
                            "networks_value": {
                                "zigbee_network": {"value": "aW1wcmVzc2l2ZV9wYXlsb2Fk"}
                            }
                        }
                    }
                },
                "analytics": {
                    "product_scenario": "IoTScenarios",
                    "origin": "Scenario",
                    "purpose": "save_iot_networks"
                }
            })")));
        const auto frame = TTypedSemanticFrameRequest{eventPayload};
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeSaveIotNetworksSemanticFrame());
    }

    Y_UNIT_TEST(ActivateGenerativeTaleWithCharacter) {
        auto makeActivateGenerativeTaleSemanticFrame = [](const TString& Character) {
            TSemanticFrame frame;
            frame.SetName("alice.generative_tale.activate");

            AddSlot("character", "string", Character, "string", frame);

            auto& typedFrame = *frame.MutableTypedSemanticFrame()->MutableActivateGenerativeTaleSemanticFrame();
            typedFrame.MutableCharacter()->SetStringValue(Character);

            return frame;
        };
        UNIT_ASSERT_MESSAGES_EQUAL(
            FrameFromTextJson(R"({
                "typed_semantic_frame": {
                    "activate_generative_tale_semantic_frame": {
                        "generative_tale_character": {
                            "string_value": "Hero"
                        }
                    }
                },
                "analytics": {
                    "product_scenario": "general_conversation",
                    "origin": "Scenario",
                    "purpose": "generative_tale"
                }
            })"),
            makeActivateGenerativeTaleSemanticFrame("Hero"));
    }


    Y_UNIT_TEST(TStartIotDiscovery) {
        const auto eventPayload = JsonToProto<google::protobuf::Struct>(JsonFromString(TStringBuf(R"({
                "typed_semantic_frame": {
                    "start_iot_discovery_semantic_frame": {
                        "request": {
                            "request_value": {
                                "protocols": ["Zigbee"]
                            }
                        },
                        "session_id": {
                            "string_value": "some-id"
                        }
                    }
                },
                "analytics": {
                    "product_scenario": "IoTScenarios",
                    "origin": "Scenario",
                    "purpose": "start_iot_discovery"
                }
            })")));
        const auto frame = TTypedSemanticFrameRequest{eventPayload};
        const auto request = MakeStartIotDiscoveryRequest();
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeStartIotDiscoverySemanticFrame(request, "some-id"));
    }

    Y_UNIT_TEST(TFinishIotDiscovery) {
        const auto eventPayload = JsonToProto<google::protobuf::Struct>(JsonFromString(TStringBuf(R"({
                "typed_semantic_frame": {
                    "finish_iot_discovery_semantic_frame": {
                        "request": {
                            "request_value": {
                                "protocols": ["Zigbee"],
                                "discovered_endpoints": [{
                                    "id": "example-lamp-id-1",
                                    "meta": {
                                        "type": "LightEndpointType"
                                    },
                                    "capabilities": [{
                                        "@type": "type.googleapis.com/NAlice.TOnOffCapability",
                                        "state": {"on": true}
                                    }]
                                }]
                            }
                        }
                    }
                },
                "analytics": {
                    "product_scenario": "IoTScenarios",
                    "origin": "Scenario",
                    "purpose": "finish_iot_discovery"
                }
            })")));
        const auto frame = TTypedSemanticFrameRequest{eventPayload};

        const auto request = MakeFinishIotDiscoveryRequest();
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeFinishIotDiscoverySemanticFrame(request));
    }

    Y_UNIT_TEST(TForgetIotEndpoints) {
        const auto eventPayload = JsonToProto<google::protobuf::Struct>(JsonFromString(TStringBuf(R"({
                "typed_semantic_frame": {
                    "forget_iot_endpoints_semantic_frame": {
                        "request": {
                            "request_value": {
                                "endpoint_ids": ["lamp-device-id-1"]
                            }
                        }
                    }
                },
                "analytics": {
                    "product_scenario": "IoTScenarios",
                    "origin": "Scenario",
                    "purpose": "forget_iot_endpoints"
                }
            })")));
        const auto frame = TTypedSemanticFrameRequest{eventPayload};

        const auto request = MakeForgetIotEndpointsRequest();
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeForgetIotEndpointsSemanticFrame(request));
    }

    Y_UNIT_TEST(TvPromoTemplateRequest) {
        const TSemanticFrame& actual = MakeTvPromoTemplateRequestSemanticFrame(true, false);
        const TStringBuf expectedRaw = R"({
            "typed_semantic_frame": {
                "tv_promo_request_semantic_frame": {
                    "chosen_template": {
                        "tandem_promo_template": {
                            "is_tandem_devices_available": true,
                            "is_tandem_connected": false
                        }
                    }
                }
            },
            "analytics": {
                "origin": "Scenario",
                "purpose": "get_tv_feature_boarding_template"
            }
        })";
        UNIT_ASSERT_MESSAGES_EQUAL(actual, FrameFromTextJson(expectedRaw));
    }

    Y_UNIT_TEST(TvReportPromoTemplateShown) {
        const TSemanticFrame& actual = MakeTvPromoTemplateShownReportSemanticFrame();
        const TStringBuf expectedRaw = R"({
            "typed_semantic_frame": {
                "tv_promo_template_shown_report_semantic_frame": {
                    "chosen_template": {"tandem_promo_template": {}},
                    "show_template_timestamp_millis":{"epoch_value":"100500"}
                }
            },
            "analytics": {
                "origin": "Scenario",
                "purpose": "report_tv_feature_boarding_template_shown"
            }
        })";
        UNIT_ASSERT_MESSAGES_EQUAL(actual, FrameFromTextJson(expectedRaw));
    }

    Y_UNIT_TEST(TUploadContactsRequest) {
        auto makeTUploadContactsRequestSemanticFrame = [](const NAlice::NData::TUpdateContactsRequest& uploadContactsRequest) {
            TSemanticFrame frame;
            frame.SetName("alice.upload_contact_request");

            AddSlot("upload_request", "alice.upload_contact_request.request", uploadContactsRequest, "alice.upload_contact_request.request", frame);

            auto typedFrame = frame.MutableTypedSemanticFrame()->MutableUploadContactsRequestSemanticFrame();
            typedFrame->MutableUploadRequest()->MutableRequestValue()->CopyFrom(uploadContactsRequest);

            return frame;
        };

        auto makeUpdateContactsRequest = []() {
            NAlice::NData::TUpdateContactsRequest request;

            auto& contactInfo = *request.MutableUpdatedContacts()->Add()->MutableTelegramContactInfo();
            contactInfo.SetUserId("44004400");
            contactInfo.SetProvider("telegram");
            contactInfo.SetContactId("33003300");
            contactInfo.SetFirstName("Маша");
            contactInfo.SetSecondName("Машина");

            return request;
        };

        UNIT_ASSERT_MESSAGES_EQUAL(
            FrameFromTextJson(R"({
                "typed_semantic_frame": {
                    "upload_contacts_request_semantic_frame": {
                        "upload_request": {
                            "request_value": {
                                "updated_contacts": [
                                    {
                                        "telegram_contact_info": {
                                            "user_id": "44004400",
                                            "provider": "telegram",
                                            "contact_id": "33003300",
                                            "first_name": "Маша",
                                            "second_name": "Машина"
                                        }
                                    }
                                ]
                            }
                        }
                    }
                },
                "analytics": {
                    "product_scenario": "Сontacts",
                    "purpose": "upload_contacts",
                    "origin": "Scenario"
                }
            })"),
            makeTUploadContactsRequestSemanticFrame(makeUpdateContactsRequest()));
    }

    Y_UNIT_TEST(TGetTvSearchResult) {
            auto makeTGetTvSearchResultSemanticFrame = [](const TString& searchText, const TString& restrictionMode, const TString& restrictionAge) {
                TSemanticFrame frame;
                frame.SetName("alice.tv.get_search_result");
                AddSlot("search_text", "string", searchText, "string", frame);
                AddSlot("restriction_mode", "string", restrictionMode, "string", frame);
                AddSlot("restriction_age", "string", restrictionAge, "string", frame);

                auto& typedFrame = *frame.MutableTypedSemanticFrame()->MutableGetTvSearchResult();
                typedFrame.MutableSearchText()->SetStringValue(searchText);
                typedFrame.MutableRestrictionMode()->SetStringValue(restrictionMode);
                typedFrame.MutableRestrictionAge()->SetStringValue(restrictionAge);
                return frame;
            };

            const TStringBuf expectedTypedSemanticFrameRaw = R"({
                "typed_semantic_frame": {
                    "get_tv_search_result": {
                        "search_text": {
                            "string_value": "test_search"
                        },
                        "restriction_mode": {
                            "string_value": "full"
                        },
                        "restriction_age": {
                            "string_value": "18"
                        }
                    }
                },
                "analytics": {
                    "origin": "Scenario",
                    "purpose": "get_search_result"
                }
            })";

            const auto &actualTypedSemanticFrame = makeTGetTvSearchResultSemanticFrame("test_search", "full", "18");
            UNIT_ASSERT_MESSAGES_EQUAL(FrameFromTextJson(expectedTypedSemanticFrameRaw), actualTypedSemanticFrame);
    }

    Y_UNIT_TEST(TGetVideoGalleries) {
            auto makeTGetVideoGalleriesSemanticFrame = [](
                    const TString& categoryId,
                    const int offset,
                    const int limit,
                    const TString& cacheHash,
                    const int maxItemsPerGallery,
                    const TString& fromScreenId,
                    const TString& parentFromScreenId,
                    const bool kidModeEnabled,
                    const TString& restrictionAge) {
                TSemanticFrame frame;
                frame.SetName("alice.video.get_galleries");
                AddSlot("category_id", "string", categoryId, "string", frame);
                AddSlot("max_items_per_gallery", "num", maxItemsPerGallery, "num", frame);
                AddSlot("offset", "num", offset, "num", frame);
                AddSlot("limit", "num", limit, "num", frame);
                AddSlot("cache_hash", "string", cacheHash, "string", frame);
                AddSlot("from_screen_id", "string", fromScreenId, "string", frame);
                AddSlot("parent_from_screen_id", "string", parentFromScreenId, "string", frame);
                AddSlot("kid_mode_enabled", "bool", kidModeEnabled, "bool", frame);
                AddSlot("restriction_age", "string", restrictionAge, "string", frame);

                auto& typedFrame = *frame.MutableTypedSemanticFrame()->MutableGetVideoGalleries();
                typedFrame.MutableCategoryId()->SetStringValue(categoryId);
                typedFrame.MutableMaxItemsPerGallery()->SetNumValue(maxItemsPerGallery);
                typedFrame.MutableOffset()->SetNumValue(offset);
                typedFrame.MutableLimit()->SetNumValue(limit);
                typedFrame.MutableCacheHash()->SetStringValue(cacheHash);
                typedFrame.MutableFromScreenId()->SetStringValue(fromScreenId);
                typedFrame.MutableParentFromScreenId()->SetStringValue(parentFromScreenId);
                typedFrame.MutableKidModeEnabled()->SetBoolValue(kidModeEnabled);
                typedFrame.MutableRestrictionAge()->SetStringValue(restrictionAge);
                return frame;
            };

            const TStringBuf expectedTypedSemanticFrameRaw = R"({
                "typed_semantic_frame": {
                    "get_video_galleries_semantic_frame": {
                        "category_id": {
                            "string_value": "test_category_id"
                        },
                        "max_items_per_gallery": {
                            "num_value": 14
                        },
                        "offset": {
                            "num_value": 15
                        },
                        "limit": {
                            "num_value": 22
                        },
                        "cache_hash": {
                            "string_value": "abcdef"
                        },
                        "from_screen_id": {
                            "string_value": "show_more"
                        },
                        "parent_from_screen_id": {
                            "string_value": "main"
                        },
                        "kid_mode_enabled": {
                            "bool_value": true
                        },
                        "restriction_age": {
                            "string_value": "6"
                        }
                    }
                },
                "analytics": {
                    "origin": "Scenario",
                    "purpose": "get_video_galleries"
                }
            })";

            const auto &actualTypedSemanticFrame = makeTGetVideoGalleriesSemanticFrame(
                    /* categoryId= */ "test_category_id",
                    /* offset= */ 15,
                    /* limit= */ 22,
                    /* cacheHash= */ "abcdef",
                    /* maxItemsPerGallery= */ 14,
                    /* fromScreenId= */ "show_more",
                    /* parentFromScreenId= */ "main",
                    /* kidModeEnabled= */ true,
                    /* restrictionAge= */ "6"
            );
            UNIT_ASSERT_MESSAGES_EQUAL(FrameFromTextJson(expectedTypedSemanticFrameRaw), actualTypedSemanticFrame);
    }

    Y_UNIT_TEST(TGetVideoGallery) {
            auto makeTGetVideoGallerySemanticFrame = [](
                    const TString& id,
                    const int offset,
                    const int limit,
                    const TString& cacheHash,
                    const TString& fromScreenId,
                    const TString& parentFromScreenId,
                    const int carouselPosition,
                    const TString& carouselTitle,
                    const bool kidModeEnabled,
                    const TString& restrictionAge) {
                TSemanticFrame frame;
                frame.SetName("alice.video.get_gallery");
                AddSlot("id", "string", id, "string", frame);
                AddSlot("offset", "num", offset, "num", frame);
                AddSlot("limit", "num", limit, "num", frame);
                AddSlot("cache_hash", "string", cacheHash, "string", frame);
                AddSlot("from_screen_id", "string", fromScreenId, "string", frame);
                AddSlot("parent_from_screen_id", "string", parentFromScreenId, "string", frame);
                AddSlot("carousel_position", "num", carouselPosition, "num", frame);
                AddSlot("carousel_title", "string", carouselTitle, "string", frame);
                AddSlot("kid_mode_enabled", "bool", kidModeEnabled, "bool", frame);
                AddSlot("restriction_age", "string", restrictionAge, "string", frame);

                auto& typedFrame = *frame.MutableTypedSemanticFrame()->MutableGetVideoGallerySemanticFrame();
                typedFrame.MutableId()->SetStringValue(id);
                typedFrame.MutableOffset()->SetNumValue(offset);
                typedFrame.MutableLimit()->SetNumValue(limit);
                typedFrame.MutableCacheHash()->SetStringValue(cacheHash);
                typedFrame.MutableFromScreenId()->SetStringValue(fromScreenId);
                typedFrame.MutableParentFromScreenId()->SetStringValue(parentFromScreenId);
                typedFrame.MutableCarouselPosition()->SetNumValue(carouselPosition);
                typedFrame.MutableCarouselTitle()->SetStringValue(carouselTitle);
                typedFrame.MutableKidModeEnabled()->SetBoolValue(kidModeEnabled);
                typedFrame.MutableRestrictionAge()->SetStringValue(restrictionAge);
                return frame;
            };

            const TStringBuf expectedTypedSemanticFrameRaw = R"({
                "typed_semantic_frame": {
                    "get_video_gallery_semantic_frame": {
                        "id": {
                            "string_value": "test_id"
                        },
                        "offset": {
                            "num_value": 17
                        },
                        "limit": {
                            "num_value": 11
                        },
                        "cache_hash": {
                            "string_value": "afthdv"
                        },
                        "from_screen_id": {
                            "string_value": "show_more"
                        },
                        "parent_from_screen_id": {
                            "string_value": "main"
                        },
                        "carousel_position": {
                            "num_value": 51
                        },
                        "carousel_title": {
                            "string_value": "Фильмы для вас"
                        },
                        "kid_mode_enabled": {
                            "bool_value": true
                        },
                        "restriction_age": {
                            "string_value": "12"
                        }
                    }
                },
                "analytics": {
                    "origin": "Scenario",
                    "purpose": "get_video_gallery"
                }
            })";

            const auto &actualTypedSemanticFrame = makeTGetVideoGallerySemanticFrame(
                /* id */ "test_id",
                /* offset */ 17,
                /* limit */ 11,
                /* cacheHash */ "afthdv",
                /* fromScreenId */ "show_more",
                /* parentFromScreenId */ "main",
                /* carouselPosition */ 51,
                /* carouselTitle */ "Фильмы для вас",
                /* kidModeEnabled */ true,
                /* restrictionAge */ "12"
            );
            UNIT_ASSERT_MESSAGES_EQUAL(FrameFromTextJson(expectedTypedSemanticFrameRaw), actualTypedSemanticFrame);
    }

    Y_UNIT_TEST(TypedSemanticFrameMusicAnnounceDisable) {
        const auto eventPayload = JsonToProto<google::protobuf::Struct>(JsonFromString(TStringBuf(R"___({
            "typed_semantic_frame": {
                "music_announce_disable_semantic_frame": {}
            },
            "analytics": {
                "origin": "Scenario",
                "purpose": "music_announce_disable"
            }
        })___")));
        const auto frame = TTypedSemanticFrameRequest{eventPayload};
        TSemanticFrame expectedFrame;
        expectedFrame.SetName("alice.music.announce.disable");
        expectedFrame.MutableTypedSemanticFrame()->MutableMusicAnnounceDisableSemanticFrame();
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, expectedFrame);
    }

    Y_UNIT_TEST(TypedSemanticFrameMusicAnnounceEnable) {
        const auto eventPayload = JsonToProto<google::protobuf::Struct>(JsonFromString(TStringBuf(R"___({
            "typed_semantic_frame": {
                "music_announce_enable_semantic_frame": {}
            },
            "analytics": {
                "origin": "Scenario",
                "purpose": "music_announce_enable"
            }
        })___")));
        const auto frame = TTypedSemanticFrameRequest{eventPayload};
        TSemanticFrame expectedFrame;
        expectedFrame.SetName("alice.music.announce.enable");
        expectedFrame.MutableTypedSemanticFrame()->MutableMusicAnnounceEnableSemanticFrame();
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, expectedFrame);
    }

    Y_UNIT_TEST(TSwitchTvChannelSemanticFrame) {
        auto makeTSwitchTvChannelSemanticFrame = [](const TString& uri) {
            TSemanticFrame frame;
            frame.SetName("alice.switch_tv_channel_sf");
            AddSlot("uri", "string", uri, "string", frame);

            auto& typedFrame = *frame.MutableTypedSemanticFrame()->MutableSwitchTvChannelSemanticFrame();
            typedFrame.MutableUri()->SetStringValue(uri);
            return frame;
        };

        const TStringBuf expectedTypedSemanticFrameRaw = R"({
            "typed_semantic_frame": {
                "switch_tv_channel_semantic_frame": {
                    "uri": {
                        "string_value": "content://android.media.tv/channel/823?input=com"
                    }
                }
            },
            "analytics": {
                "origin": "Scenario",
                "purpose": "switch_tv_channel"
            }
        })";

        const auto &actualTypedSemanticFrame = makeTSwitchTvChannelSemanticFrame(
            "content://android.media.tv/channel/823?input=com"
        );
        UNIT_ASSERT_MESSAGES_EQUAL(FrameFromTextJson(expectedTypedSemanticFrameRaw), actualTypedSemanticFrame);
    }

    Y_UNIT_TEST(TypedSemanticFrameConvert) {
        auto makeConvertSemanticFrame = [](const TString& typeFrom, const TString& typeTo, double amountFrom) {
            TSemanticFrame frame{};
            frame.SetName("personal_assistant.scenarios.convert");
            AddSlot("type_from", "currency", typeFrom, frame);
            AddSlot("type_to", "currency", typeTo, frame);
            AddSlot("amount_from", "num", amountFrom, frame);

            auto& typedFrame = *frame.MutableTypedSemanticFrame()->MutableConvertSemanticFrame();
            typedFrame.MutableTypeFrom()->SetCurrencyValue(typeFrom);
            typedFrame.MutableTypeTo()->SetCurrencyValue(typeTo);
            typedFrame.MutableAmountFrom()->SetNumValue(amountFrom);
            return frame;
        };

        const TStringBuf expectedTypedSemanticFrameRaw = R"({
            "typed_semantic_frame": {
                "convert_semantic_frame": {
                    "type_from": {
                        "currency_value": "USD"
                    },
                    "type_to": {
                        "currency_value": "RUR"
                    },
                    "amount_from": {
                        "num_value": "1"
                    }
                }
            },
            "analytics": {
                "origin": "Scenario",
                "purpose": "convert"
            }
        })";

        const auto &actualTypedSemanticFrame = makeConvertSemanticFrame("USD", "RUR", 1);
        UNIT_ASSERT_MESSAGES_EQUAL(FrameFromTextJson(expectedTypedSemanticFrameRaw), actualTypedSemanticFrame);
    }

    Y_UNIT_TEST(TMusicOnboardingSemanticFrame) {
        const TStringBuf expectedTypedSemanticFrameRaw = R"({
            "typed_semantic_frame": {
                "music_onboarding_semantic_frame": {}
            },
            "analytics": {
                "origin": "Scenario",
                "purpose": "music_onboarding"
            }
        })";

        const auto actualTypedSemanticFrame = MakeMusicOnboardingSemanticFrame();
        UNIT_ASSERT_MESSAGES_EQUAL(FrameFromTextJson(expectedTypedSemanticFrameRaw), actualTypedSemanticFrame);
    }

    Y_UNIT_TEST(TMusicOnboardingArtistsSemanticFrame) {
        const TStringBuf expectedTypedSemanticFrameRaw = R"({
            "typed_semantic_frame": {
                "music_onboarding_artists_semantic_frame": {}
            },
            "analytics": {
                "origin": "Scenario",
                "purpose": "music_onboarding_artists"
            }
        })";

        const auto actualTypedSemanticFrame = MakeMusicOnboardingArtistsSemanticFrame();
        UNIT_ASSERT_MESSAGES_EQUAL(FrameFromTextJson(expectedTypedSemanticFrameRaw), actualTypedSemanticFrame);
    }

    Y_UNIT_TEST(TMusicOnboardingGenresSemanticFrame) {
        const TStringBuf expectedTypedSemanticFrameRaw = R"({
            "typed_semantic_frame": {
                "music_onboarding_genres_semantic_frame": {}
            },
            "analytics": {
                "origin": "Scenario",
                "purpose": "music_onboarding_genres"
            }
        })";

        const auto actualTypedSemanticFrame = MakeMusicOnboardingGenresSemanticFrame();
        UNIT_ASSERT_MESSAGES_EQUAL(FrameFromTextJson(expectedTypedSemanticFrameRaw), actualTypedSemanticFrame);
    }

    Y_UNIT_TEST(TMusicOnboardingTracksSemanticFrame) {
        const TStringBuf expectedTypedSemanticFrameRaw = R"({
            "typed_semantic_frame": {
                "music_onboarding_tracks_semantic_frame": {}
            },
            "analytics": {
                "origin": "Scenario",
                "purpose": "music_onboarding_tracks"
            }
        })";

        const auto actualTypedSemanticFrame = MakeMusicOnboardingTracksSemanticFrame();
        UNIT_ASSERT_MESSAGES_EQUAL(FrameFromTextJson(expectedTypedSemanticFrameRaw), actualTypedSemanticFrame);
    }

    Y_UNIT_TEST(TMusicOnboardingTracksReaskSemanticFrame) {
        const TStringBuf expectedTypedSemanticFrameRaw = R"({
            "typed_semantic_frame": {
                "music_onboarding_tracks_reask_semantic_frame": {
                    "track_id": {
                        "string_value": "27420070"
                    }
                }
            },
            "analytics": {
                "origin": "Scenario",
                "purpose": "music_onboarding_tracks_reask"
            }
        })";

        const auto actualTypedSemanticFrame = MakeMusicOnboardingTracksReaskSemanticFrame("27420070");
        UNIT_ASSERT_MESSAGES_EQUAL(FrameFromTextJson(expectedTypedSemanticFrameRaw), actualTypedSemanticFrame);
    }

    Y_UNIT_TEST(GetEqualizerSettingsSemanticFrame) {
        const TStringBuf expectedTypedSemanticFrameRaw = R"({
            "typed_semantic_frame": {
                "get_equalizer_settings_semantic_frame": {
                }
            },
            "analytics": {
                "origin": "Scenario",
                "purpose": "music_onboarding_tracks_reask"
            }
        })";

        TSemanticFrame actualTypedSemanticFrame;
        actualTypedSemanticFrame.SetName("alice.get_equalizer_settings");
        actualTypedSemanticFrame.MutableTypedSemanticFrame()->MutableGetEqualizerSettingsSemanticFrame();

        UNIT_ASSERT_MESSAGES_EQUAL(FrameFromTextJson(expectedTypedSemanticFrameRaw), actualTypedSemanticFrame);
    }

    Y_UNIT_TEST(GetOpenTandemSettingSemanticFrame) {
        const TStringBuf expectedTypedSemanticFrameRaw = R"({
            "typed_semantic_frame": {
                "open_tandem_setting_semantic_frame": {
                }
            },
            "analytics": {
                "origin": "Scenario",
                "purpose": "settings"
            }
        })";

        TSemanticFrame actualTypedSemanticFrame;
        actualTypedSemanticFrame.SetName("alice.setting.tandem.open");
        actualTypedSemanticFrame.MutableTypedSemanticFrame()->MutableOpenTandemSettingSemanticFrame();

        UNIT_ASSERT_MESSAGES_EQUAL(FrameFromTextJson(expectedTypedSemanticFrameRaw), actualTypedSemanticFrame);
    }

    Y_UNIT_TEST(GetOpenSmartSpeakerSettingSemanticFrame) {
        const TStringBuf expectedTypedSemanticFrameRaw = R"({
            "typed_semantic_frame": {
                "open_smart_speaker_setting_semantic_frame": {
                }
            },
            "analytics": {
                "origin": "Scenario",
                "purpose": "settings"
            }
        })";

        TSemanticFrame actualTypedSemanticFrame;
        actualTypedSemanticFrame.SetName("alice.setting.smart_speaker.open");
        actualTypedSemanticFrame.MutableTypedSemanticFrame()->MutableOpenSmartSpeakerSettingSemanticFrame();

        UNIT_ASSERT_MESSAGES_EQUAL(FrameFromTextJson(expectedTypedSemanticFrameRaw), actualTypedSemanticFrame);
    }

    Y_UNIT_TEST(TOnboardingGetGreetingsSemanticFrame) {
        const TStringBuf payload = R"({
            "typed_semantic_frame": {
                "onboarding_get_greetings_semantic_frame": {}
            },
            "analytics": {
                "origin": "SearchApp",
                "purpose": "onboarding_in_cloud"
            }
        })";
        TSemanticFrame actualTypedSemanticFrame;
        actualTypedSemanticFrame.SetName("alice.onboarding.get_greetings");
        actualTypedSemanticFrame.MutableTypedSemanticFrame()->MutableOnboardingGetGreetingsSemanticFrame();
        UNIT_ASSERT_MESSAGES_EQUAL(FrameFromTextJson(payload), actualTypedSemanticFrame);
    }

    Y_UNIT_TEST(TVideoCallSetFavoritesSemanticFrame) {
        auto makeVideoCallSetFavoritesSemanticFrame = [](const NAlice::NData::TProviderContactList& providerContactList) {
            TSemanticFrame frame;
            frame.SetName("alice.video_call_set_favorites");

            AddSlot("user_id", "string", "33003300", "string", frame);
            AddSlot("favorites", "contact_list", providerContactList, "contact_list", frame);

            auto typedFrame = frame.MutableTypedSemanticFrame()->MutableVideoCallSetFavoritesSemanticFrame();
            typedFrame->MutableUserId()->SetStringValue("33003300");
            typedFrame->MutableFavorites()->MutableContactList()->CopyFrom(providerContactList);

            return frame;
        };

        auto makeProviderContactList = []() {
            NAlice::NData::TProviderContactList contacts;

            contacts.MutableContactData()->Add()->MutableTelegramContactData()->SetUserId("44004400");
            contacts.MutableContactData()->Add()->MutableTelegramContactData()->SetUserId("55005500");

            return contacts;
        };

        UNIT_ASSERT_MESSAGES_EQUAL(
            FrameFromTextJson(R"({
                "typed_semantic_frame": {
                    "video_call_set_favorites_semantic_frame": {
                        "user_id": {
                            "string_value": "33003300"
                        },
                        "favorites": {
                            "contact_list": {
                                "contact_data": [
                                    {
                                        "telegram_contact_data": {
                                            "user_id": "44004400"
                                        }
                                    },
                                    {
                                        "telegram_contact_data": {
                                            "user_id": "55005500"
                                        }
                                    },
                                ]
                            }
                        }
                    }
                },
                "analytics": {
                    "product_scenario": "VideoCall",
                    "purpose": "alice.video_call_set_favorites",
                    "origin": "SmartSpeaker"
                }
            })"),
            makeVideoCallSetFavoritesSemanticFrame(makeProviderContactList()));
    }

    Y_UNIT_TEST(TVideoCallIncomingSemanticFrame) {
        auto makeVideoCallIncomingSemanticFrame = [](const NAlice::NData::TProviderContactData& providerContactData) {
            TSemanticFrame frame;
            frame.SetName("alice.video_call_incoming");

            AddSlot("call_id", "string", "call_id", "string", frame);
            AddSlot("user_id", "string", "33003300", "string", frame);
            AddSlot("caller", "contact_data", providerContactData, "contact_data", frame);

            auto typedFrame = frame.MutableTypedSemanticFrame()->MutableVideoCallIncomingSemanticFrame();
            typedFrame->MutableCallId()->SetStringValue("call_id");
            typedFrame->MutableUserId()->SetStringValue("33003300");
            typedFrame->MutableCaller()->MutableContactData()->CopyFrom(providerContactData);

            return frame;
        };

        auto makeProviderContactData = []() {
            NAlice::NData::TProviderContactData contact;
            contact.MutableTelegramContactData()->SetUserId("44004400");
            return contact;
        };

        UNIT_ASSERT_MESSAGES_EQUAL(
            FrameFromTextJson(R"({
                "typed_semantic_frame": {
                    "video_call_incoming_semantic_frame": {
                        "call_id": {
                            "string_value": "call_id"
                        },
                        "user_id": {
                            "string_value": "33003300"
                        },
                        "caller": {
                            "contact_data": {
                                "telegram_contact_data": {
                                    "user_id": "44004400"
                                }
                            }
                        }
                    }
                },
                "analytics": {
                    "product_scenario": "VideoCall",
                    "purpose": "alice.video_call_incoming",
                    "origin": "SmartSpeaker"
                }
            })"),
            makeVideoCallIncomingSemanticFrame(makeProviderContactData()));
    }

    Y_UNIT_TEST(PutMoneyOnPhoneSemanticFrame) {
            TSemanticFrame putMoneyOnPhoneSemanticFrame;
            putMoneyOnPhoneSemanticFrame.SetName("alice.put_money_on_phone");

            AddSlot("amount", "sys.num", "100", "sys.num", putMoneyOnPhoneSemanticFrame);
            AddSlot("phone_number", "string", "+79991234567", "string", putMoneyOnPhoneSemanticFrame);


            auto typedFrame = putMoneyOnPhoneSemanticFrame.MutableTypedSemanticFrame()->MutablePutMoneyOnPhoneSemanticFrame();
            typedFrame->MutableAmount()->SetNumValue(100);
            typedFrame->MutablePhoneNumber()->SetStringValue("+79991234567");

            UNIT_ASSERT_MESSAGES_EQUAL(
                FrameFromTextJson(R"({
                    "typed_semantic_frame": {
                        "put_money_on_phone_semantic_frame": {
                            "phone_number": {
                                "string_value": "+79991234567"
                            },
                            "amount": {
                                "num_value": 100
                            }
                        }
                    },
                    "analytics": {
                        "product_scenario": "ImplicitSkillDiscovery",
                        "purpose": "alice.put_money_on_phone",
                        "origin": "SmartSpeaker"
                    }
                })"),
                putMoneyOnPhoneSemanticFrame);
    }

    Y_UNIT_TEST(TExternalSkillEpisodeForShowRequestSemanticFrame) {
            TSemanticFrame aliceExternalSkillEpisodeForShowRequestSemanticFrame;
            aliceExternalSkillEpisodeForShowRequestSemanticFrame.SetName("alice.external_skill_episode_for_show_request");

            AddSlot("skill_id", "string", "abcd-1234", "string", aliceExternalSkillEpisodeForShowRequestSemanticFrame);


            auto typedFrame = aliceExternalSkillEpisodeForShowRequestSemanticFrame.MutableTypedSemanticFrame()->
                MutableExternalSkillEpisodeForShowRequestSemanticFrame();
            typedFrame->MutableSkillId()->SetStringValue("abcd-1234");

            UNIT_ASSERT_MESSAGES_EQUAL(
                FrameFromTextJson(R"({
                    "typed_semantic_frame": {
                        "external_skill_episode_for_show_request_semantic_frame": {
                            "skill_id": {
                                "string_value": "abcd-1234"
                            }
                        }
                    },
                    "analytics": {
                        "product_scenario": "Dialogovo",
                        "purpose": "alice.external_skill_episode_for_show_request",
                        "origin": "SmartSpeaker"
                    }
                })"),
                aliceExternalSkillEpisodeForShowRequestSemanticFrame);
    }

    Y_UNIT_TEST(TTvLongTapTutorialSemanticFrame) {
        const TStringBuf payload = R"({
            "typed_semantic_frame": {
                "tv_long_tap_tutorial_semantic_frame": {}
            },
            "analytics": {
                "origin": "SmartTv",
                "purpose": "long_tap_tutorial",
                "product_scenario": "TVPultPromo"
            }
        })";
        TSemanticFrame actualTypedSemanticFrame;
        actualTypedSemanticFrame.SetName("alice.tv.long_tap_tutorial");
        actualTypedSemanticFrame.MutableTypedSemanticFrame()->MutableTvLongTapTutorialSemanticFrame();
        UNIT_ASSERT_MESSAGES_EQUAL(FrameFromTextJson(payload), actualTypedSemanticFrame);
    }

    Y_UNIT_TEST(TOnboardingWhatCanYouDoSemanticFrame) {
        const TStringBuf payload = R"({
            "typed_semantic_frame": {
                "onboarding_what_can_you_do_semantic_frame": {
                    "phrase_index": {
                        "uint32_value": 1
                    }
                }
            },
            "analytics": {
                "origin": "SmartSpeaker",
                "purpose": "onboarding_what_can_you_do"
            }
        })";

        TSemanticFrame frame;
        frame.SetName("alice.onboarding.what_can_you_do");
        AddSlot("phrase_index", "uint32", 1, "uint32", frame);
        auto& typedframe = *frame.MutableTypedSemanticFrame()->MutableOnboardingWhatCanYouDoSemanticFrame();
        typedframe.MutablePhraseIndex()->SetUInt32Value(1);
        UNIT_ASSERT_MESSAGES_EQUAL(FrameFromTextJson(payload), frame);
    }

    Y_UNIT_TEST(TPlayerRemoveLikeSemanticFrame) {
        const TStringBuf payload = R"({
            "typed_semantic_frame": {
                "player_remove_like_semantic_frame": {
                    "content_id": {
                        "content_id_value": {
                            "type": "Album",
                            "id": "3475523"
                        }
                    }
                }
            },
            "analytics": {
                "origin": "Scenario",
                "product_scenario": "Music",
                "purpose": "music_player_remove_like"
            }
        })";

        TSemanticFrame frame;
        frame.SetName("alice.music.remove_like");
        AddSlot("content_id", "content_id", R"({"type":"Album","id":"3475523"})", "content_id", frame);

        auto& typedFrame = *frame.MutableTypedSemanticFrame()->MutablePlayerRemoveLikeSemanticFrame();
        auto& contentId = *typedFrame.MutableContentId()->MutableContentIdValue();
        contentId.SetType(NData::NMusic::TContentId_EContentType_Album);
        contentId.SetId("3475523"); // albumId of "Never Gonna Give You Up"

        UNIT_ASSERT_MESSAGES_EQUAL(FrameFromTextJson(payload), frame);
    }

    Y_UNIT_TEST(TPlayerRemoveDislikeSemanticFrame) {
        const TStringBuf payload = R"({
            "typed_semantic_frame": {
                "player_remove_dislike_semantic_frame": {
                    "content_id": {
                        "content_id_value": {
                            "type": "Album",
                            "id": "3475523"
                        }
                    }
                }
            },
            "analytics": {
                "origin": "Scenario",
                "product_scenario": "Music",
                "purpose": "music_player_remove_dislike"
            }
        })";

        TSemanticFrame frame;
        frame.SetName("alice.music.remove_dislike");
        AddSlot("content_id", "content_id", R"({"type":"Album","id":"3475523"})", "content_id", frame);

        auto& typedFrame = *frame.MutableTypedSemanticFrame()->MutablePlayerRemoveDislikeSemanticFrame();
        auto& contentId = *typedFrame.MutableContentId()->MutableContentIdValue();
        contentId.SetType(NData::NMusic::TContentId_EContentType_Album);
        contentId.SetId("3475523"); // albumId of "Never Gonna Give You Up"

        UNIT_ASSERT_MESSAGES_EQUAL(FrameFromTextJson(payload), frame);
    }

    Y_UNIT_TEST(TGallerySelectSemanticFrame) {
        const TStringBuf payload = R"({
            "typed_semantic_frame": {
                "gallery_video_select_semantic_frame": {
                    "action": {
                        "string_value": "play"
                    },
                    "provider_item_id": {
                        "string_value": "491469b7d6be16479e53cc893f3ed2ba"
                    }
                }
            },
            "analytics": {
                "origin": "Scenario",
                "product_scenario": "Video",
                "purpose": "select_video_from_gallery"
            }
        })";

        TSemanticFrame frame;
        frame.SetName("alice.tv.gallery_video_select");
        AddSlot("action", "string", "play", "string", frame);
        AddSlot("provider_item_id", "string", "491469b7d6be16479e53cc893f3ed2ba", "string", frame);

        auto typedFrame = frame.MutableTypedSemanticFrame()->MutableGalleryVideoSelectSemanticFrame();
        typedFrame->MutableAction()->SetStringValue("play");
        typedFrame->MutableProviderItemId()->SetStringValue("491469b7d6be16479e53cc893f3ed2ba");

        UNIT_ASSERT_MESSAGES_EQUAL(FrameFromTextJson(payload), frame);
    }

    Y_UNIT_TEST(TMusicPlayFixlistSemanticFrame) {
        const TStringBuf payload = R"({
            "typed_semantic_frame": {
                "music_play_fixlist_semantic_frame": {
                    "special_answer_info": {
                        "fixlist_info_value": "{\"answer_type\":\"track\",\"title\":\"Белка и волк\",\"id\":\"740880\"}"
                    }
                }
            },
            "analytics": {
                "origin": "Scenario",
                "product_scenario": "Music",
                "purpose": "fixlist"
            }
        })";

        TSemanticFrame frame;
        frame.SetName("personal_assistant.scenarios.music_play_fixlist");
        AddSlot("special_answer_info",
                "custom.music.fixlist.info",
                "{\"answer_type\":\"track\",\"title\":\"Белка и волк\",\"id\":\"740880\"}",
                "custom.music.fixlist.info",
                frame);

        auto& typedFrame = *frame.MutableTypedSemanticFrame()->MutableMusicPlayFixlistSemanticFrame();
        typedFrame.MutableSpecialAnswerInfo()->SetFixlistInfoValue("{\"answer_type\":\"track\",\"title\":\"Белка и волк\",\"id\":\"740880\"}");

        UNIT_ASSERT_MESSAGES_EQUAL(FrameFromTextJson(payload), frame);
    }

    Y_UNIT_TEST(TMusicPlayAnaphoraSemanticFrame) {
        const TStringBuf payload = R"({
            "typed_semantic_frame": {
                "music_play_anaphora_semantic_frame": {
                    "action_request": {
                        "action_request_value": "autoplay"
                    },
                    "repeat": {
                        "repeat_value": "repeat"
                    },
                    "target_type": {
                        "target_type_value": "album"
                    },
                    "need_similar": {
                        "need_similar_value": "need_similar"
                    },
                    "order": {
                        "order_value": "shuffle"
                    }
                }
            },
            "analytics": {
                "origin": "Scenario",
                "product_scenario": "Music",
                "purpose": "anaphora"
            }
        })";

        TSemanticFrame frame;
        frame.SetName("personal_assistant.scenarios.music_play_anaphora");
        AddSlot("action_request", "custom.action_request", "autoplay", "custom.action_request", frame);
        AddSlot("repeat", "custom.repeat", "repeat", "custom.repeat", frame);
        AddSlot("target_type", "custom.target_type", "album", "custom.target_type", frame);
        AddSlot("need_similar", "custom.need_similar", "need_similar", "custom.need_similar", frame);
        AddSlot("order", "custom.order", "shuffle", "custom.order", frame);

        auto& typedFrame = *frame.MutableTypedSemanticFrame()->MutableMusicPlayAnaphoraSemanticFrame();
        typedFrame.MutableActionRequest()->SetActionRequestValue(TActionRequestSlot_EValue_autoplay);
        typedFrame.MutableRepeat()->SetRepeatValue(TRepeatSlot_EValue_repeat);
        typedFrame.MutableTargetType()->SetTargetTypeValue(TTargetTypeSlot_EValue_album);
        typedFrame.MutableNeedSimilar()->SetNeedSimilarValue(TNeedSimilarSlot_EValue_need_similar);
            typedFrame.MutableOrder()->SetOrderValue(TOrderSlot_EValue_shuffle);

        UNIT_ASSERT_MESSAGES_EQUAL(FrameFromTextJson(payload), frame);
    }

    Y_UNIT_TEST(TMusicPlayFairytaleSemanticFrame) {
        const TStringBuf payload = R"({
            "typed_semantic_frame": {
                "music_play_fairytale_semantic_frame": {
                    "fairytale_theme": {
                        "fairytale_theme_value": "bedtime"
                    }
                }
            },
            "analytics": {
                "origin": "Scenario",
                "product_scenario": "Music",
                "purpose": "fairytale"
            }
        })";

        TSemanticFrame frame;
        frame.SetName("personal_assistant.scenarios.music_play_fairytale");
        AddSlot("fairytale_theme", "custom.fairytale_theme", "bedtime", "custom.fairytale_theme", frame);

        auto& typedFrame = *frame.MutableTypedSemanticFrame()->MutableMusicPlayFairytaleSemanticFrame();
        typedFrame.MutableFairytaleTheme()->SetFairytaleThemeValue(TFairytaleThemeSlot_EValue_bedtime);

        UNIT_ASSERT_MESSAGES_EQUAL(FrameFromTextJson(payload), frame);
    }

    Y_UNIT_TEST(TStartMultiroomSemanticFrame) {
        const TStringBuf payload = R"({
            "typed_semantic_frame": {
                "start_multiroom_semantic_frame": {
                    "location_room": [
                        {
                            "user_iot_room_value": "test_room_1"
                        },
                        {
                            "user_iot_room_value": "test_room_2"
                        }
                    ],
                    "location_group": [
                        {
                            "user_iot_group_value": "test_group"
                        }
                    ],
                    "location_device": [
                        {
                            "user_iot_device_value": "test_device_1"
                        },
                        {
                            "user_iot_device_value": "test_device_2"
                        },
                        {
                            "user_iot_device_value": "test_device_3"
                        }
                    ],
                    "location_everywhere": {
                        "user_iot_multiroom_all_devices_value": "test_everywhere"
                    }
                }
            },
            "analytics": {
                "origin": "Scenario",
                "product_scenario": "Music",
                "purpose": "start_multiroom"
            }
        })";

        TSemanticFrame frame;
        frame.SetName("alice.multiroom.start_multiroom");
        AddSlot("location_room", "user.iot.room", "test_room_1", "user.iot.room", frame);
        AddSlot("location_room", "user.iot.room", "test_room_2", "user.iot.room", frame);
        AddSlot("location_group", "user.iot.group", "test_group", "user.iot.group", frame);
        AddSlot("location_device", "user.iot.device", "test_device_1", "user.iot.device", frame);
        AddSlot("location_device", "user.iot.device", "test_device_2", "user.iot.device", frame);
        AddSlot("location_device", "user.iot.device", "test_device_3", "user.iot.device", frame);
        AddSlot("location_everywhere", "user.iot.multiroom_all_devices", "test_everywhere", "user.iot.multiroom_all_devices", frame);

        auto& typedFrame = *frame.MutableTypedSemanticFrame()->MutableStartMultiroomSemanticFrame();
        typedFrame.AddLocationRoom()->SetUserIotRoomValue("test_room_1");
        typedFrame.AddLocationRoom()->SetUserIotRoomValue("test_room_2");
        typedFrame.AddLocationGroup()->SetUserIotGroupValue("test_group");
        typedFrame.AddLocationDevice()->SetUserIotDeviceValue("test_device_1");
        typedFrame.AddLocationDevice()->SetUserIotDeviceValue("test_device_2");
        typedFrame.AddLocationDevice()->SetUserIotDeviceValue("test_device_3");
        typedFrame.MutableLocationEverywhere()->SetUserIotMultiroomAllDevicesValue("test_everywhere");

        UNIT_ASSERT_MESSAGES_EQUAL(FrameFromTextJson(payload), frame);
    }
} // Y_UNIT_TEST_SUITE

} // namespace

Y_DECLARE_OUT_SPEC(, NAlice::NData::NAliceShow::TDayPart_EValue, stream, value) {
    stream << NAlice::NData::NAliceShow::TDayPart_EValue_Name(value);
}
Y_DECLARE_OUT_SPEC(, NAlice::NData::NAliceShow::TAge_EValue, stream, value) {
    stream << NAlice::NData::NAliceShow::TAge_EValue_Name(value);
}

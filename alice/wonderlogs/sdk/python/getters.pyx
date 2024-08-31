import json

from util.generic.maybe cimport TMaybe
from util.generic.string cimport TString, TStringBuf
from util.generic.vector cimport TVector

from libcpp cimport bool as cpp_bool

cdef extern from 'variant' namespace 'std':
    cdef cppclass variant3 'std::variant' [T1, T2, T3]:
        pass
    
    cdef cppclass monostate:
        pass

    cpp_bool holds_alternative3 'std::holds_alternative' [T, T1, T2, T3] (const variant3[T1, T2, T3]& v)
    const T& get3 'std::get' [T, T1, T2, T3] (const variant3[T1, T2, T3]& v)


cdef extern from 'alice/megamind/protos/analytics/megamind_analytics_info.pb.h' namespace 'NAlice::NMegamind':
    cdef cppclass TMegamindAnalyticsInfo:
        TMegamindAnalyticsInfo() except +
        cpp_bool ParseFromString(const TString&)


cdef extern from 'google/protobuf/message.h' namespace 'google::protobuf':
    cdef cppclass Message:
        pass


cdef extern from 'google/protobuf/util/json_util.h' namespace 'google::protobuf::util':
    cdef cppclass JsonOptions:
        JsonOptions()


cdef extern from 'alice/megamind/protos/common/frame.pb.h' namespace 'NAlice':
    cdef cppclass TSemanticFrame_TSlot(Message):
        TSemanticFrame_TSlot() except +
        cpp_bool ParseFromString(const TString&)


cdef extern from 'alice/library/json/json.h' namespace 'NAlice':
    cdef TString JsonStringFromProto(const Message& message, const JsonOptions& jsonOptions) except +


cdef extern from 'alice/library/client/protos/client_info.pb.h' namespace 'NAlice':
    cdef cppclass TClientInfoProto:
        TClientInfoProto() except +
        cpp_bool ParseFromString(const TString&)


cdef extern from 'alice/megamind/protos/speechkit/response.pb.h' namespace 'NAlice':
    cdef cppclass TSpeechKitResponseProto_TResponse_TCard:
        TSpeechKitResponseProto_TResponse_TCard() except +
        cpp_bool ParseFromString(const TString&)


cdef extern from 'library/cpp/json/writer/json_value.h' namespace 'NJson':
    cdef cppclass TJsonValue:
        pass


cdef extern from 'library/cpp/json/json_reader.h' namespace 'NJson':
    cdef TJsonValue ReadJsonFastTree(TStringBuf, cpp_bool notClosedBracketIsError) except +


cdef extern from 'library/cpp/json/json_writer.h' namespace 'NJson':
    cdef TString WriteJson(const TJsonValue&) except +


cdef extern from 'alice/wonderlogs/sdk/core/getters.h' namespace 'NAlice::NWonderSdk':
    cdef cppclass TCard:
        TJsonValue ToJson()
    cdef TMaybe[TString] GetIntent(const TMegamindAnalyticsInfo& analyticsInfo)
    cdef TMaybe[TString] GetProductScenarioName(const TMegamindAnalyticsInfo& analyticsInfo)
    cdef TMaybe[TString] GetMusicAnswerType(const TMegamindAnalyticsInfo& analyticsInfo)
    cdef TMaybe[TString] GetMusicGenre(const TMegamindAnalyticsInfo& analyticsInfo)
    cdef TMaybe[TString] GetFiltersGenre(const TMegamindAnalyticsInfo& analyticsInfo)
    cdef cpp_bool SmartHomeUser(const TMegamindAnalyticsInfo& analyticsInfo)
    cdef TString GetApp(const TClientInfoProto& appInfo)
    cdef TString GetPlatform(const TClientInfoProto& appInfo)
    cdef TString GetVersion(const TClientInfoProto& appInfo)
    cdef double GetSoundLevel(const TMaybe[double]& soundLevel, const TStringBuf appId)
    cdef TMaybe[TString] GetPath(const TMegamindAnalyticsInfo& analyticsInfo,
                                 const TJsonValue& callbackArgs)
    cdef cpp_bool FormChanged(const TMegamindAnalyticsInfo& analyticsInfo,
                              const TJsonValue& callbackArgs)
    cdef TVector[TSemanticFrame_TSlot] GetSlots(const TMegamindAnalyticsInfo& analyticsInfo)
    cdef variant3[monostate, TString, TJsonValue] GetSlotValue(const TSemanticFrame_TSlot& slot)
    cdef TVector[TCard] ParseCards(const TVector[TSpeechKitResponseProto_TResponse_TCard]& cards)


cdef to_python_str(const TString& s):
    return s.c_str()[:s.length()].decode('utf-8')


cdef TString _to_TString(s):
    if not s:
        return TString()

    assert isinstance(s, basestring), 'Value: {}, Type: {}'.format(s, type(s))
    if isinstance(s, unicode):
        s = s.encode('utf-8')
    return TString(<const char*>s, len(s))


cdef TJsonValue _to_TJsonValue(json):
    return ReadJsonFastTree(_to_TString(json), <cpp_bool> True)


cdef TMegamindAnalyticsInfo _parse_analytics_info(serialized_analytics_info):
    cdef TMegamindAnalyticsInfo megamind_analytics_info = TMegamindAnalyticsInfo()
    megamind_analytics_info.ParseFromString(serialized_analytics_info)
    return megamind_analytics_info


cdef TClientInfoProto _parse_app(serialized_app):
    cdef TClientInfoProto app = TClientInfoProto()
    app.ParseFromString(serialized_app)
    return app


cdef TSemanticFrame_TSlot _parse_slot(serialized_slot):
    cdef TSemanticFrame_TSlot slot = TSemanticFrame_TSlot()
    slot.ParseFromString(serialized_slot)
    return slot


cdef TSpeechKitResponseProto_TResponse_TCard _parse_card(serialized_card):
    cdef TSpeechKitResponseProto_TResponse_TCard card = TSpeechKitResponseProto_TResponse_TCard()
    card.ParseFromString(serialized_card)
    return card


def get_intent(serialized_analytics_info):
    cdef TMaybe[TString] res = GetIntent(_parse_analytics_info(serialized_analytics_info))
    if res.Defined():
        return to_python_str(res.GetRef())
    return None


def get_product_scenario_name(serialized_analytics_info):
    cdef TMaybe[TString] res = GetProductScenarioName(_parse_analytics_info(serialized_analytics_info))
    if res.Defined():
        return to_python_str(res.GetRef())
    return None


def get_music_answer_type(serialized_analytics_info):
    cdef TMaybe[TString] res = GetMusicAnswerType(_parse_analytics_info(serialized_analytics_info))
    if res.Defined():
        return to_python_str(res.GetRef())
    return None


def get_music_genre(serialized_analytics_info):
    cdef TMaybe[TString] res = GetMusicGenre(_parse_analytics_info(serialized_analytics_info))
    if res.Defined():
        return to_python_str(res.GetRef())
    return None


def get_filters_genre(serialized_analytics_info):
    cdef TMaybe[TString] res = GetFiltersGenre(_parse_analytics_info(serialized_analytics_info))
    if res.Defined():
        return to_python_str(res.GetRef())
    return None


def smart_home_user(serialized_analytics_info):
    return SmartHomeUser(_parse_analytics_info(serialized_analytics_info))


def get_app(serialized_app):
    cdef TString res = GetApp(_parse_app(serialized_app))
    return to_python_str(res)


def get_platform(serialized_app):
    cdef TString res = GetPlatform(_parse_app(serialized_app))
    return to_python_str(res)


def get_version(serialized_app):
    cdef TString res = GetVersion(_parse_app(serialized_app))
    return to_python_str(res)


def get_sound_level(sound_level, app_id):
    cdef TMaybe[double] sound_level_maybe
    if sound_level is not None:
        sound_level_maybe = TMaybe[double](float(sound_level))
    cdef TMaybe[double] res = GetSoundLevel(sound_level_maybe, _to_TString(app_id))
    if res.Defined():
        return res.GetRef()
    return None


def get_path(serialized_analytics_info, callback_args):
    cdef TMaybe[TString] res = GetPath(_parse_analytics_info(serialized_analytics_info),
                                       _to_TJsonValue(callback_args))
    if res.Defined():
        return to_python_str(res.GetRef())
    return None


def form_changed(serialized_analytics_info, callback_args):
    return FormChanged(_parse_analytics_info(serialized_analytics_info),
                       _to_TJsonValue(callback_args))


def get_slots(serialized_analytics_info):
    cdef TVector[TSemanticFrame_TSlot] slots = GetSlots(_parse_analytics_info(serialized_analytics_info))
    dict_slots = []
    for slot in slots:
        dict_slots.append(json.loads(to_python_str(JsonStringFromProto(slot, JsonOptions()))))
    return dict_slots


def get_slot_value(serialized_slot):
    cdef variant3[monostate, TString, TJsonValue] value = GetSlotValue(_parse_slot(serialized_slot))
    if holds_alternative3[TString, monostate, TString, TJsonValue](value):
        return to_python_str(get3[TString, monostate, TString, TJsonValue](value))
    elif holds_alternative3[TJsonValue, monostate, TString, TJsonValue](value):
        return json.loads(to_python_str(WriteJson(get3[TJsonValue, monostate, TString, TJsonValue](value))))
    return None

def parse_cards(serialized_cards):
    cdef TVector[TSpeechKitResponseProto_TResponse_TCard] cards
    for serialized_card in serialized_cards:
        cards.push_back(_parse_card(serialized_card))
    cdef TVector[TCard] parsed_cards = ParseCards(cards)

    parsed_cards_dict = []

    for parsed_card in parsed_cards:
        card_dict = json.loads(to_python_str(WriteJson(parsed_card.ToJson())))
        parsed_cards_dict.append(card_dict)
    
    return parsed_cards_dict

_Alice = 'alice.'


_MovieSuggest = _Alice + 'movie_suggest.'

ShowCartoons = _MovieSuggest + 'show_cartoons'
ShowMovies = _MovieSuggest + 'show_movies'


_Vinsless = _Alice + 'vinsless.'

_HandcraftedVinsless = _Vinsless + 'handcrafted.'

DoYouBelieveInGod = _HandcraftedVinsless + 'do_you_believe_in_god'
DoYouHaveChildren = _HandcraftedVinsless + 'do_you_have_children'
LetsHaveASmoke = _HandcraftedVinsless + 'lets_have_a_smoke'
SendMeYourPhoto = _HandcraftedVinsless + 'send_me_your_photo'
WhoAmI = _HandcraftedVinsless + 'who_am_i'
VinslessWhoIsYourMaker = _HandcraftedVinsless + 'who_is_your_maker'
ReciteAPoem = _HandcraftedVinsless + 'recite_a_poem'
VinslessHowAreYou = _HandcraftedVinsless + 'how_are_you'
DoYouKnowOtherAssistants = _HandcraftedVinsless + 'do_you_know_other_assistants'
VinslessWhatCanYouDo = _HandcraftedVinsless + 'what_can_you_do'

_VinslessMusic = _Vinsless + 'music.'
Meditation = _VinslessMusic + 'meditation'
MorningShow = _VinslessMusic + 'morning_show'

_AliceMusic = _Alice + 'music.'
MusicAnnounceEnable = _AliceMusic + 'announce.enable'
MusicComplexLikeDislike = _AliceMusic + 'complex_like_dislike'
MusicSendSongText = _AliceMusic + 'send_song_text'
MusicWhatAlbumIsThisSongFrom = _AliceMusic + 'what_album_is_this_song_from'
MusicWhatYearIsThisSong = _AliceMusic + 'what_year_is_this_song'

MusicOnboarding = _Alice + 'music_onboarding'
MusicOnboardingArtists = MusicOnboarding + '.artists'
MusicOnboardingGenres = MusicOnboarding + '.genres'
MusicOnboardingTracks = MusicOnboarding + '.tracks'

GenerativeTale = _Alice + 'generative_tale'
GenerativeTaleAskCharacter = GenerativeTale + '_ask_character'
GenerativeTaleBanlistActivation = GenerativeTale + '.banlist.activation'

ActivateRadioNews = 'external_skill.activate_news_from_provider'


_Automotive = 'personal_assistant.automotive.'

Greeting = _Automotive + 'greeting'


_Handcrafted = 'personal_assistant.handcrafted.'

AutoAppConfirmationYes = _Handcrafted + 'autoapp.confirmation_yes'
AutoAppConfirmationNo = _Handcrafted + 'autoapp.confirmation_no'
Blind = _Handcrafted + 'blind'
Cancel = _Handcrafted + 'cancel'
Deaf = _Handcrafted + 'deaf'
GoodMorning = _Handcrafted + 'good_morning'
GoodNight = _Handcrafted + 'goodnight'
Harassment = _Handcrafted + 'harassment'
HeadsOrTails = _Handcrafted + 'heads_or_tails'
Hello = _Handcrafted + 'hello'
HowAreYou = _Handcrafted + 'how_are_you'
TellAboutYandexAuto = _Handcrafted + 'tell_about_yandex_auto'
TellMeAJoke = _Handcrafted + 'tell_me_a_joke'
WhatIsYourFavoriteApplication = _Handcrafted + 'what_is_your_favorite_application'
WhatIsYourName = _Handcrafted + 'what_is_your_name'
WhereDoChildrenComeFrom = _Handcrafted + 'where_do_children_come_from'
WhoIsThere = _Handcrafted + 'who_is_there'
WhoIsYourMaker = _Handcrafted + 'who_is_your_maker'


_Internal = 'personal_assistant.internal.'

Bugreport = _Internal + 'bugreport'
BugreportContinue = Bugreport + '__continue'
BugreportDeactivate = Bugreport + '__deactivate'

Hardcoded = _Internal + 'hardcoded'


GeneralConversation = 'personal_assistant.general_conversation.general_conversation'
GeneralConversationDummy = GeneralConversation + '_dummy'
GeneralConversationOriginal = 'alice.general_conversation.general_conversation'
GeneralConversationMovieAkinator = 'movie_akinator'
GeneralConversationVinsless = _Vinsless + 'general_conversation'
GeneralConversationVinslessDummy = GeneralConversationVinsless + '.general_conversation_dummy'
GeneralConversationVinslessPardon = GeneralConversationVinsless + '.beg_your_pardon'


_Navi = 'personal_assistant.navi.'

AddPoint = _Navi + 'add_point'
ChangeVoice = _Navi + 'change_voice'
ParkingRoute = _Navi + 'parking_route'
ResetRoute = _Navi + 'reset_route'
NaviHideLayer = _Navi + 'hide_layer'
NaviShowLayer = _Navi + 'show_layer'
NaviShowRouteOnMap = _Navi + 'show_route_on_map'
WhenWeGetThere = _Navi + 'when_we_get_there'
HowLongToDrive = _Navi + 'how_long_to_drive'


_Scenarios = 'personal_assistant.scenarios.'
_MmScenarios = 'mm.' + _Scenarios

AlarmAskTime = _Scenarios + 'alarm_ask_time'
AlarmAskSound = _Scenarios + 'alarm_ask_sound'
AlarmCancel = _Scenarios + 'alarm_cancel'
AlarmCancelEllipsis = AlarmCancel + '__ellipsis'
AlarmHowToSetSound = _Scenarios + 'alarm_how_to_set_sound'
AlarmSet = _Scenarios + 'alarm_set'
AlarmSetSound = _Scenarios + 'alarm_set_sound'
AlarmSetWithSound = _Scenarios + 'alarm_set_with_sound'
AlarmSoundSetLevel = _Scenarios + 'alarm_sound_set_level'
AlarmShow = _Scenarios + 'alarm_show'
AlarmWhatSoundLevelIsSet = _Scenarios + 'alarm_what_sound_level_is_set'

BluetoothOn = _Scenarios + 'bluetooth_on'
BluetoothOff = _Scenarios + 'bluetooth_off'

Call = _Scenarios + 'call'

CommonFeedbackNegative = 'personal_assistant.feedback.feedback_negative'
CommonFeedbackPositive = 'personal_assistant.feedback.feedback_positive'

Convert = _Scenarios + 'convert'

CreateReminder = _Scenarios + 'create_reminder'
CreateReminderCancel = CreateReminder + '__cancel'
CreateReminderEllipsis = CreateReminder + '__ellipsis'
CreateTodo = _Scenarios + 'create_todo'
ListTodo = _Scenarios + 'list_todo'

ConnectNamedLocationToDevice = _Scenarios + 'connect_named_location_to_device'

EmergencyCall = 'emergency_call'

ExternalSkill = _Scenarios + 'external_skill'
ExternalSkillGc = ExternalSkill + '_gc'

FindPoi = _Scenarios + 'find_poi'
FindPoiDetails = FindPoi + '__details'
FindPoiScrollNext = FindPoi + '__scroll__next'

GamesOnboarding = _Scenarios + 'games_onboarding'

GetDate = _Scenarios + 'get_date'

GetMyLocation = _Scenarios + 'get_my_location'

GetNews = _Scenarios + 'get_news'
GetNewsDetails = GetNews + '__details'
GetNewsEllipsis = GetNews + '__ellipsis'
GetNewsMore = GetNews + '__more'

GetTime = _Scenarios + 'get_time'
GetTimeEllipsis = GetTime + '__ellipsis'

GetWeather = _Scenarios + 'get_weather'
GetWeatherDetails = GetWeather + '__details'
GetWeatherEllipsis = GetWeather + '__ellipsis'
GetWeatherFast = GetWeather + '.fast'

GetWeatherNowcast = _Scenarios + 'get_weather_nowcast'
GetWeatherNowcastEllipsis = GetWeatherNowcast + '__ellipsis'
GetWeatherNowcastFast = GetWeatherNowcast + '.fast'

GetWeatherNowcastPrecMap = 'alice.scenarios.get_weather_nowcast_prec_map'
GetWeatherNowcastPrecMapEllipsis = GetWeatherNowcastPrecMap + '__ellipsis'

GetWeatherPressure = 'alice.scenarios.get_weather_pressure'
GetWeatherPressureEllipsis = GetWeatherPressure + '__ellipsis'

GetWeatherWind = 'alice.scenarios.get_weather_wind'
GetWeatherWindEllipsis = GetWeatherWind + '__ellipsis'

HowMuch = _Scenarios + 'how_much'
HowMuchEllipsis = HowMuch + '__ellipsis'
ProtocolHowMuch = 'how_much'

ImageWhatIsThis = _Alice + 'image_what_is_this_'
ImageBarcode = ImageWhatIsThis + 'barcode'
ImageClothes = ImageWhatIsThis + 'clothes'
ImageDark = ImageWhatIsThis + 'dark'
ImageEntity = ImageWhatIsThis + 'entity'
ImageFace = ImageWhatIsThis + 'face'
ImageGruesome = ImageWhatIsThis + 'gruesome'
ImageMarket = ImageWhatIsThis + 'market'
ImageMuseum = ImageWhatIsThis + 'museum'
ImageOcr = ImageWhatIsThis + 'ocr'
ImageOcrVoice = ImageWhatIsThis + 'ocr_voice'
ImagePorn = ImageWhatIsThis + 'porn'
ImageSimilar = ImageWhatIsThis + 'similar'
ImageSimilarArtwork = ImageWhatIsThis + 'similar_artwork'
ImageSimilarPeople = ImageWhatIsThis + 'similar_people'
ImageSimilarPeopleFrontal = ImageWhatIsThis + 'frontal_similar_people'
ImageTranslate = ImageWhatIsThis + 'translate'
ImageOfficeLens = ImageWhatIsThis + 'office_lens'
ImageSimilarLike = ImageWhatIsThis + 'similarlike'
ImageInfo = ImageWhatIsThis + 'info'

Market = _Scenarios + 'market'
MarketCancel = Market + '__cancel'
MarketStartAgain = Market + '__start_choice_again'
MarketContinue = Market + '__market'
MarketEllipsis = MarketContinue + '__ellipsis'
MarketGarbage = Market + '__garbage'
MarketCheckout = Market + '__checkout'

MarketOrdersStatus = _Scenarios + 'market_orders_status'
MarketProtocolOrdersStatus = 'orders_status'
MarketProtocolUserLoggedIn = 'user_logged_in'

MovieDiscussFactQuestion = 'alice.general_conversation.general_conversation.alice.movie_discuss'
MovieDiscussOpinionQuestion = 'alice.general_conversation.yes_i_watched_it.movie_question'
LetsDiscussSpecificMovie = 'alice.general_conversation.lets_discuss_specific_movie'
LetsDiscussSomeMovie = 'alice.general_conversation.lets_discuss_some_movie'

ListReminders = _Scenarios + 'list_reminders'

MusicFairyTale = _Scenarios + 'music_fairy_tale'
MusicPlay = _Scenarios + 'music_play'
MusicPlayAnaphora = _Scenarios + 'music_play_anaphora'
MusicPlayLess = _Scenarios + 'music_play_less'
MusicWhatIsPlaying = _Scenarios + 'music_what_is_playing'
MusicSingSong = _Scenarios + "music_sing_song"
MusicSingSongNext = MusicSingSong + '__next'
MusicPodcast = _Scenarios + 'music_podcast'
MusicAmbientSound = _Scenarios + 'music_ambient_sound'

Mail = 'mail'
MailGoThrough = Mail + '.go_through'
NewMail = Mail + '.write'
PushSent = Mail + '.push_sent'
MailUnsupportedSurface = Mail + '.unsupported_surface'

NotificationsRead = _Alice + 'notifications_read'
NotificationsSubscribe = _Alice + 'notifications_subscribe'
NotificationsUnsubscribe = _Alice + 'notifications_unsubscribe'
NotificationsSubscriptionsList = _Alice + 'notifications_subscriptions_list'

Onboarding = _Scenarios + 'onboarding'
OnboardingNext = Onboarding + '__next'
OnboardingCancel = Onboarding + '__cancel'

OnboardingConfigureSuccess = _Alice + 'onboarding.starting_configure_success'
OnboardingCriticalUpdate = _Alice + 'onboarding.starting_critical_update'
OnboardingImageSearch = _Scenarios + 'onboarding_image_search'

OnboardingWhatCanYouDo = _Alice + 'onboarding.what_can_you_do'

OpenSiteOrApp = _Scenarios + 'open_site_or_app'

PlayerContinue = _Scenarios + 'player_continue'
PlayerLike = _Scenarios + 'player_like'
PlayerDislike = _Scenarios + 'player_dislike'
PlayerPause = _Scenarios + 'player_pause'
PlayerRepeat = _Scenarios + 'player_repeat'
PlayerReplay = _Scenarios + 'player_replay'
PlayerShuffle = _Scenarios + 'player_shuffle'
PlayNextTrack = _Scenarios + 'player_next_track'
PlayPreviousTrack = _Scenarios + 'player_previous_track'

ProhibitionError = _Scenarios + 'common.prohibition_error'

PureGeneralConversation = _Scenarios + 'pure_general_conversation'
PureGeneralConversationDeactivation = PureGeneralConversation + '_deactivation'
PureGeneralConversationFeedbackPositive = 'personal_assistant.feedback.gc_feedback_positive'
PureGeneralConversationFeedbackNegative = 'personal_assistant.feedback.gc_feedback_negative'
PureGeneralConversationFeedbackNeutral = 'personal_assistant.feedback.gc_feedback_neutral'

ProtocolRadioPlay = 'radio_play'
RadioPlay = _Scenarios + 'radio_play'
RadioPlayOnboarding = _Scenarios + 'radio_play_onboarding'
RadioPlayOnboardingNext = RadioPlayOnboarding + '__next'

RecurringPurchase = _Scenarios + 'recurring_purchase'
RecurringPurchaseEllipsis = RecurringPurchase + '__ellipsis'
RecurringPurchaseLogin = RecurringPurchase + '__login'

Repeat = _Scenarios + 'repeat'
RepeatAfterMe = _Scenarios + 'repeat_after_me'

_Taximeter = _Alice + 'taximeter.'
TaximeterRequestconfirm = _Taximeter + 'requestconfirm_order_offer'

TaxiNewDisabled = _Scenarios + 'taxi_new_disabled'
TaxiNewOrder = _Scenarios + 'taxi_new_order'
TaxiNewOrderChangePaymentOrTariff = TaxiNewOrder + '__change_payment_or_tariff'
TaxiNewStatus = _Scenarios + 'taxi_new_status'
TaxiNewOrderConfirmationYes = TaxiNewOrder + '__confirmation_yes'
TaxiNewOrderSpecify = _Scenarios + 'taxi_new_order__specify'
TaxiNewOrderConfirmationNo = _Scenarios + 'taxi_new_order__confirmation_no'
TaxiNewOrderConfirmationWrong = _Scenarios + 'taxi_new_order__confirmation_wrong'

TestHardcodedResponse = 'test_hardcoded_response'

TimerCancel = _Scenarios + 'timer_cancel'
TimerSet = _Scenarios + 'timer_set'
TimerShow = _Scenarios + 'timer_show'

Translate = _Scenarios + 'translate'
TranslateEllipsis = Translate + '__ellipsis'
TranslateQuicker = Translate + '__quicker'
TranslateSlower = Translate + '__slower'

ProtocolTranslate = 'translate'

TvBroadcast = _Scenarios + 'tv_broadcast'
TvBroadcastEllipsis = TvBroadcast + '__ellipsis'
TvStream = _Scenarios + 'tv_stream'

Search = _Scenarios + 'search'
DeviceShortcut = 'device_shortcut'
PhoneCall = 'phone_call'
ProtocolSearch = 'search'
Factoid = 'factoid'
FactoidSrc = 'factoid_src'
FactoidCall = 'factoid_call'
ObjectAnswer = 'object'
ObjectSearchOO = 'object_search_oo'
Calculator = 'calculator'
Nav = 'nav'
ShortcutPrefix = 'shortcut'
Serp = 'serp'

SearchFilterReset = _Scenarios + 'search_filter_reset'
SearchFilterSetFamily = _Scenarios + 'search_filter_set_family'
SearchFilterSetNoFilter = _Scenarios + 'search_filter_set_no_filter'

ShoppingListAdd = _Scenarios + 'shopping_list_add'

ShortcutTvSecretPromo = ShortcutPrefix + '.tv_secret_promo'

ShowRoute = _Scenarios + 'show_route'
ShowRouteEllipsis = ShowRoute + '__ellipsis'
ShowRouteOnMap = ShowRoute + '__show_route_on_map'

ShowTraffic = _Scenarios + 'show_traffic'
ShowTrafficEllipsis = ShowTraffic + '__ellipsis'

SkillRecommendation = _Scenarios + 'skill_recommendation'

SkipVideoFragment = _Scenarios + 'skip_video_fragment'

VideoPlay = _MmScenarios + 'video_play'

VoiceprintEnroll = _Scenarios + 'voiceprint_enroll'
VoiceprintEnrollCollectVoice = VoiceprintEnroll + '__collect_voice'
VoiceprintEnrollFinish = VoiceprintEnroll + '__finish'

WhatCanYouDo = _Scenarios + 'what_can_you_do'

WhatIsThis = _Scenarios + 'image_what_is_this'

_Quasar = _Scenarios + 'quasar.'
GoHome = _Quasar + 'go_home'
GoDown = _Quasar + 'go_down'
GoTop = _Quasar + 'go_top'
GoUp = _Quasar + 'go_up'
ProtocolGoHome = 'go_home'

_MmQuasar = _MmScenarios + 'quasar.'
OpenCurrentVideo = _MmQuasar + 'open_current_video'

PlayerRewind = _Scenarios + 'player_rewind'

MmVideoPlay = _MmScenarios + 'video_play'

RememberNamedLocation = _Scenarios + 'remember_named_location'
RememberNamedLocationEllipsis = RememberNamedLocation + '__ellipsis'

_Whisper = 'whisper.'
WhisperSaySomething = _Whisper + 'say_something'
WhisperTurnOn = _Whisper + 'turn_on'
WhisperTurnOff = _Whisper + 'turn_off'
WhisperWhatIsIt = _Whisper + 'what_is_it'

_Metronome = 'alice.metronome.'
MetronomeStart = _Metronome + 'start'
MetronomeSlower = _Metronome + 'slower'
MetronomeFaster = _Metronome + 'faster'

SoundSetLevel = _Scenarios + 'sound_set_level'

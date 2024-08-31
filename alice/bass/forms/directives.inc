// Computer vision
CREATE_AND_REGISTER_DIRECTIVE(TCVRedirectDirective, TStringBuf("computer_vision_redirect"))
CREATE_AND_REGISTER_DIRECTIVE(TCVImageRecognizerOnCameraDirective, TStringBuf("computer_vision_image_recognizer"))

////////////////////////////////////////////////////////////////////////////////

// Avia
CREATE_AND_REGISTER_DIRECTIVE(TAviaCheckoutSiteDirective, TStringBuf("avia_checkout"))

////////////////////////////////////////////////////////////////////////////////

// Bluetooth
CREATE_AND_REGISTER_DIRECTIVE(TBluetoothStartDirective, TStringBuf("bluetooth_start"))
CREATE_AND_REGISTER_DIRECTIVE(TBluetoothStopDirective, TStringBuf("bluetooth_stop"))

////////////////////////////////////////////////////////////////////////////////

// Thereminvox

CREATE_AND_REGISTER_DIRECTIVE(TThereminvoxStartDirective, TStringBuf("start_thereminvox"))
CREATE_AND_REGISTER_DIRECTIVE(TThereminvoxStopDirective, TStringBuf("stop_thereminvox"))

////////////////////////////////////////////////////////////////////////////////

// Client command

CREATE_AND_REGISTER_DIRECTIVE(TClientGoForwardDirective, TStringBuf("quasar_go_forward"))
CREATE_AND_REGISTER_DIRECTIVE(TClientGoBackwardDirective, TStringBuf("quasar_go_backward"))
CREATE_AND_REGISTER_DIRECTIVE(TClientGoUpDirective, TStringBuf("quasar_go_up"))
CREATE_AND_REGISTER_DIRECTIVE(TClientGoDownDirective, TStringBuf("quasar_go_down"))
CREATE_AND_REGISTER_DIRECTIVE(TClientGoTopDirective, TStringBuf("quasar_go_top"))
CREATE_AND_REGISTER_DIRECTIVE(TClientGoHomeDirective, TStringBuf("quasar_go_home"))
CREATE_AND_REGISTER_DIRECTIVE(TClientGoToBeginningDirective, TStringBuf("quasar_go_to_the_beginning"))
CREATE_AND_REGISTER_DIRECTIVE(TClientGoToEndDirective, TStringBuf("quasar_go_to_the_end"))
CREATE_AND_REGISTER_DIRECTIVE(TOpenUriDirective, TStringBuf("open_uri"))
CREATE_AND_REGISTER_DIRECTIVE(TClientUnknownCommandDirective, TStringBuf("client_unknown_command"))
CREATE_AND_REGISTER_DIRECTIVE(TShowPromoDirective, TStringBuf("show_promo"))

////////////////////////////////////////////////////////////////////////////////

// External skills
CREATE_AND_REGISTER_DIRECTIVE(TExternalSkillEndDialogSessionDirective, TStringBuf("external_skill_end_dialog_session"))
CREATE_AND_REGISTER_DIRECTIVE(TExternalSkillCloseDialogDirective, TStringBuf("external_skill_close_dialog"))
CREATE_AND_REGISTER_DIRECTIVE(TExternalSkillPlayerPauseDirective, TStringBuf("external_skill_player_pause"))
CREATE_AND_REGISTER_DIRECTIVE(TExternalSkillDescriptionUpdateDialogInfoDirective, TStringBuf("external_skill_update_dialog_info"))

////////////////////////////////////////////////////////////////////////////////

// Navigator
CREATE_AND_REGISTER_DIRECTIVE(TNavigatorSetPlaceDirective, TStringBuf("navi_set_place"))
CREATE_AND_REGISTER_DIRECTIVE(TNavigatorAddPointDirective, TStringBuf("navi_add_point"))
CREATE_AND_REGISTER_DIRECTIVE(TNavigatorSwitch2FasterAlternativeDirective, TStringBuf("navi_switch_to_faster_alternative"))
CREATE_AND_REGISTER_DIRECTIVE(TNavigatorMapSearchDirective, TStringBuf("navi_map_search"))
CREATE_AND_REGISTER_DIRECTIVE(TNavigatorShowPositionDirective, TStringBuf("navi_show_user_position"))
CREATE_AND_REGISTER_DIRECTIVE(TNavigatorAuth4RefuelDirective, TStringBuf("navi_auth_for_refuel"))
CREATE_AND_REGISTER_DIRECTIVE(TNavigatorRefuelDirective, TStringBuf("navi_tanker"))
CREATE_AND_REGISTER_DIRECTIVE(TNavigatorShowMapDirective, TStringBuf("navi_show_ui_map"))
CREATE_AND_REGISTER_DIRECTIVE(TNavigatorShowGuidanceDirective, TStringBuf("navi_show_ui_map_travel"))
CREATE_AND_REGISTER_DIRECTIVE(TNavigatorExternalConfirmationDirective, TStringBuf("navi_external_confirmation"))
CREATE_AND_REGISTER_DIRECTIVE(TNavigatorBuildRouteDirective, TStringBuf("navi_build_route_on_map"))
CREATE_AND_REGISTER_DIRECTIVE(TNavigatorSetSettingsDirective, TStringBuf("navi_set_setting"))
CREATE_AND_REGISTER_DIRECTIVE(TNavigatorShowPointOnMapDirective, TStringBuf("navi_show_point_on_map"))
CREATE_AND_REGISTER_DIRECTIVE(TNavigatorUpdateSettingDirective, TStringBuf("navi_update_flag_setting"))
CREATE_AND_REGISTER_DIRECTIVE(TNavigatorLayerTrafficDirective, TStringBuf("navi_layer_traffic"))
CREATE_AND_REGISTER_DIRECTIVE(TNavigatorParkingRouteDirective, TStringBuf("navi_carparks_route"))
CREATE_AND_REGISTER_DIRECTIVE(TNavigatorShowRouteDirective, TStringBuf("navi_show_route_overview"))
CREATE_AND_REGISTER_DIRECTIVE(TNavigatorResetRouteDirective, TStringBuf("navi_clear_route"))
CREATE_AND_REGISTER_DIRECTIVE(TNavigatorSearchGasStationsDirective, TStringBuf("navi_search_gas_stations"))
CREATE_AND_REGISTER_DIRECTIVE(TNavigatorChangeVoiceDirective, TStringBuf("navi_set_sound_scheme"))
CREATE_AND_REGISTER_DIRECTIVE(TNavigatorUnknownSchemeDirective, TStringBuf("navi_unknown_scheme"))

////////////////////////////////////////////////////////////////////////////////

// Taxi
CREATE_AND_REGISTER_DIRECTIVE(TTaxiCall2DriverDirective, TStringBuf("taxi_call_2_driver"))
CREATE_AND_REGISTER_DIRECTIVE(TTaxiCall2SupportDirective, TStringBuf("taxi_call_to_support"))
CREATE_AND_REGISTER_DIRECTIVE(TTaxiOpenAppWithOrderDirective, TStringBuf("taxi_open_app_with_order"))
CREATE_AND_REGISTER_DIRECTIVE(TTaxiRedirectToPassportDirective, TStringBuf("taxi_redirect_to_passport"))
CREATE_AND_REGISTER_DIRECTIVE(TTaxiShowLegalDirective, TStringBuf("taxi_show_legal"))

////////////////////////////////////////////////////////////////////////////////

// Auto
CREATE_AND_REGISTER_DIRECTIVE(TAutoMediaControlDirective, TStringBuf("yandexauto_media_control"))
CREATE_AND_REGISTER_DIRECTIVE(TAutoLaunchDirective, TStringBuf("yandexauto_launch"))
CREATE_AND_REGISTER_DIRECTIVE(TAutoSoundDirective, TStringBuf("yandexauto_sound"))
CREATE_AND_REGISTER_DIRECTIVE(TAutoFmRadioDirective, TStringBuf("yandexauto_fm_radio"))

////////////////////////////////////////////////////////////////////////////////

// Player
CREATE_AND_REGISTER_DIRECTIVE(TPlayerRewindDirective, TStringBuf("player_rewind"))
CREATE_AND_REGISTER_DIRECTIVE(TChangeAudioDirective, TStringBuf("change_audio"))
CREATE_AND_REGISTER_DIRECTIVE(TChangeSubtitlesDirective, TStringBuf("change_subtitles"))
CREATE_AND_REGISTER_DIRECTIVE(TVideoPlayerRewindDirective, TStringBuf("video_player_rewind"))
CREATE_AND_REGISTER_DIRECTIVE(TMusicPlayerContinueDirective, TStringBuf("music_player_continue"))
CREATE_AND_REGISTER_DIRECTIVE(TBluetoothPlayerContinueDirective, TStringBuf("bluetooth_player_continue"))
CREATE_AND_REGISTER_DIRECTIVE(TVideoPlayerContinueDirective, TStringBuf("video_player_continue"))
CREATE_AND_REGISTER_DIRECTIVE(TPlayerPauseDirective, TStringBuf("player_pause"))
CREATE_AND_REGISTER_DIRECTIVE(TClearQueueDirective, TStringBuf("clear_queue"))
CREATE_AND_REGISTER_DIRECTIVE(TAudioStopDirective, TStringBuf("audio_stop"))
CREATE_AND_REGISTER_DIRECTIVE(TMusicPlayerPauseDirective, TStringBuf("music_player_pause"))
CREATE_AND_REGISTER_DIRECTIVE(TVoicePrintPlayerPauseDirective, TStringBuf("voiceprint_player_pause"))
CREATE_AND_REGISTER_DIRECTIVE(TPlayerOrderDirective, TStringBuf("player_order"))
CREATE_AND_REGISTER_DIRECTIVE(TPlayerShuffleDirective, TStringBuf("player_shuffle"))
CREATE_AND_REGISTER_DIRECTIVE(TPlayerRepeatDirective, TStringBuf("player_repeat"))
CREATE_AND_REGISTER_DIRECTIVE(TPlayerReplayDirective, TStringBuf("player_replay"))
CREATE_AND_REGISTER_DIRECTIVE(TPlayerNextTrackDirective, TStringBuf("player_next_track"))
CREATE_AND_REGISTER_DIRECTIVE(TPlayerPrevTrackDirective, TStringBuf("player_previous_track"))
CREATE_AND_REGISTER_DIRECTIVE(TPlayerDislikeDirective, TStringBuf("player_dislike"))
CREATE_AND_REGISTER_DIRECTIVE(TPlayerLikeDirective, TStringBuf("player_like"))
CREATE_AND_REGISTER_DIRECTIVE(TPlayerUnknownCommandDirective, TStringBuf("player_unknown_command"))
CREATE_AND_REGISTER_DIRECTIVE(TShowVideoSettingsDirective, TStringBuf("show_video_settings"));

////////////////////////////////////////////////////////////////////////////////

// Radio
CREATE_AND_REGISTER_DIRECTIVE(TRadioSmartSpeakerPlayDirective, TStringBuf("radio_smart_speaker_play"))
CREATE_AND_REGISTER_DIRECTIVE(TRadioAppPlayDirective, TStringBuf("radio_app_or_site_play"))

////////////////////////////////////////////////////////////////////////////////

// Sound
CREATE_AND_REGISTER_DIRECTIVE(TSoundMuteDirective, TStringBuf("sound_mute"))
CREATE_AND_REGISTER_DIRECTIVE(TSoundUnmuteDirective, TStringBuf("sound_unmute"))
CREATE_AND_REGISTER_DIRECTIVE(TSoundLoaderDirective, TStringBuf("sound_louder"))
CREATE_AND_REGISTER_DIRECTIVE(TSoundQuiterDirective, TStringBuf("sound_quiter"))
CREATE_AND_REGISTER_DIRECTIVE(TSoundSetLevelDirective, TStringBuf("sound_set_level"))

////////////////////////////////////////////////////////////////////////////////

// Search Filter
CREATE_AND_REGISTER_DIRECTIVE(TSearchFilterSetDirective, TStringBuf("set_search_filter"))

////////////////////////////////////////////////////////////////////////////////

// Music
CREATE_AND_REGISTER_DIRECTIVE(TMusicRecognizerDirective, TStringBuf("music_start_recognizer"))
CREATE_AND_REGISTER_DIRECTIVE(TYaMusicOpenTrackFromShazamDirective, TStringBuf("yamusic_open_track_page_from_shazam"))
CREATE_AND_REGISTER_DIRECTIVE(TYaMusicRecognitionDirective, TStringBuf("music_recognition"))
CREATE_AND_REGISTER_DIRECTIVE(TMusicAmbientSoundDirective, TStringBuf("music_ambient_sound"))
CREATE_AND_REGISTER_DIRECTIVE(TMusicHardcodedSoundDirective, TStringBuf("music_hardcoded_sound"))
CREATE_AND_REGISTER_DIRECTIVE(TMusicSmartSpeakerPlayDirective, TStringBuf("music_smart_speaker_play"))
CREATE_AND_REGISTER_DIRECTIVE(TMusicAppPlayDirective, TStringBuf("music_app_or_site_play"))
CREATE_AND_REGISTER_DIRECTIVE(TMusicInternalPlayerPlayDirective, TStringBuf("music_internal_player_play"))
CREATE_AND_REGISTER_DIRECTIVE(TMusicVerticalShowDirective, TStringBuf("music_vertical_show"))
CREATE_AND_REGISTER_DIRECTIVE(TStartMultiroomDirective, TStringBuf("start_multiroom"))

////////////////////////////////////////////////////////////////////////////////

// Video
CREATE_AND_REGISTER_DIRECTIVE(TVideoAddDebugInfoDirective, TStringBuf("video_show_debug_info"))
CREATE_AND_REGISTER_DIRECTIVE(TVideoShowDescriptionDirective, TStringBuf("video_show_description"))
CREATE_AND_REGISTER_DIRECTIVE(TVideoShowGalleryDirective, TStringBuf("video_show_gallery"))
CREATE_AND_REGISTER_DIRECTIVE(TVideoShowSeasonGalleryDirective, TStringBuf("video_show_season_gallery"))
CREATE_AND_REGISTER_DIRECTIVE(TVideoShowPayPushScreenDirective, TStringBuf("video_show_pay_push_screen"))
CREATE_AND_REGISTER_DIRECTIVE(TVideoPlayDirective, TStringBuf("video_play"))
CREATE_AND_REGISTER_DIRECTIVE(TVideoPlayViaUriDirective, TStringBuf("video_play_via_uri"))
CREATE_AND_REGISTER_DIRECTIVE(TVideoOpenYaVideoDirective, TStringBuf("video_search_video"))
CREATE_AND_REGISTER_DIRECTIVE(TVideoOpenYaSearchDirective, TStringBuf("video_search"))
CREATE_AND_REGISTER_DIRECTIVE(TWebOSLaunchAppDirective, TStringBuf("web_os_launch_app"))
CREATE_AND_REGISTER_DIRECTIVE(TWebOSShowGalleryDirective, TStringBuf("web_os_show_gallery"))

////////////////////////////////////////////////////////////////////////////////

// TV Channel
CREATE_AND_REGISTER_DIRECTIVE(TShowGalleryDirective, TStringBuf("show_gallery"))
CREATE_AND_REGISTER_DIRECTIVE(TTvChannelsShowGalleryDirective, TStringBuf("tv_show_gallery"))

////////////////////////////////////////////////////////////////////////////////

// General Conversation
CREATE_AND_REGISTER_DIRECTIVE(TScreenOffDirective, TStringBuf("screen_off_from_microintent"))
CREATE_AND_REGISTER_DIRECTIVE(TGeneralConversationPlayerPauseDirective, TStringBuf("general_conversation_player_pause"))

////////////////////////////////////////////////////////////////////////////////

// Market
CREATE_AND_REGISTER_DIRECTIVE(TMarketOpenSerpSearchDirective, TStringBuf("market_open_serp_search"))
CREATE_AND_REGISTER_DIRECTIVE(TMarketGoToShopDirective, TStringBuf("market_go_to_shop"))

////////////////////////////////////////////////////////////////////////////////

// Reports
CREATE_AND_REGISTER_DIRECTIVE(TBugReportSendDirective, TStringBuf("bug_report_send"))
CREATE_AND_REGISTER_DIRECTIVE(TOpenFeedbackDirective, TStringBuf("open_feedback"))

////////////////////////////////////////////////////////////////////////////////

// Navigation
CREATE_AND_REGISTER_DIRECTIVE(TNavigationOpenSoftDirective, TStringBuf("navigation_open_soft"))
CREATE_AND_REGISTER_DIRECTIVE(TNavigationOpenSiteDirective, TStringBuf("navigation_open_site"))
CREATE_AND_REGISTER_DIRECTIVE(TNavigationOpenAppOrSiteDirective, TStringBuf("navigation_open_app_or_site"))

////////////////////////////////////////////////////////////////////////////////

// Phone
CREATE_AND_REGISTER_DIRECTIVE(TPhoneCallDirective, TStringBuf("phone_call"))
CREATE_AND_REGISTER_DIRECTIVE(TPhoneFindContactsDirective, TStringBuf("phone_find_contacts"))
CREATE_AND_REGISTER_DIRECTIVE(TPhoneSendMessageDirective, TStringBuf("phone_send_message"))

////////////////////////////////////////////////////////////////////////////////

// Alarm
CREATE_AND_REGISTER_DIRECTIVE(TAlarmShowDirective, TStringBuf("alarm_show"))
CREATE_AND_REGISTER_DIRECTIVE(TAlarmStopPlayingDirective, TStringBuf("alarm_stop"))
CREATE_AND_REGISTER_DIRECTIVE(TAlarmStopPlayingOnQuasarDirective, TStringBuf("alarm_stop_on_quasar"))
CREATE_AND_REGISTER_DIRECTIVE(TAlarmUpdateDirective, TStringBuf("alarm_update"))
CREATE_AND_REGISTER_DIRECTIVE(TAlarmNewDirective, TStringBuf("alarm_new"))
CREATE_AND_REGISTER_DIRECTIVE(TAlarmOneCancelDirective, TStringBuf("alarm_one_cancel"))
CREATE_AND_REGISTER_DIRECTIVE(TAlarmManyCancelDirective, TStringBuf("alarm_many_cancel"))
CREATE_AND_REGISTER_DIRECTIVE(TAlarmAllCancelDirective, TStringBuf("alarm_all_cancel"))
CREATE_AND_REGISTER_DIRECTIVE(TAlarmSetSoundDirective, TStringBuf("alarm_set_sound"))
CREATE_AND_REGISTER_DIRECTIVE(TAlarmResetSoundDirective, TStringBuf("alarm_reset_sound"))
CREATE_AND_REGISTER_DIRECTIVE(TAlarmSetMaxLevelDirective, TStringBuf("alarm_set_max_level"))
CREATE_AND_REGISTER_DIRECTIVE(TDrawScledAnimationsDirective, TStringBuf("draw_scled_animations"))

////////////////////////////////////////////////////////////////////////////////

// Reminder
CREATE_AND_REGISTER_DIRECTIVE(TReminderShowDirective, TStringBuf("reminder_show"))
CREATE_AND_REGISTER_DIRECTIVE(TRemindersSetDirective, TStringBuf("reminders_set_directive"))
CREATE_AND_REGISTER_DIRECTIVE(TRemindersCancelDirective, TStringBuf("reminders_cancel_directive"))
CREATE_AND_REGISTER_DIRECTIVE(TRequestPermissionsDirective, TStringBuf("request_permissions"))

////////////////////////////////////////////////////////////////////////////////

// ToDoDirective
CREATE_AND_REGISTER_DIRECTIVE(TToDoShowReminderDirective, TStringBuf("todo_reminder_show"))

////////////////////////////////////////////////////////////////////////////////

// Timer
CREATE_AND_REGISTER_DIRECTIVE(TTimerShowDirective, TStringBuf("timer_show"))
CREATE_AND_REGISTER_DIRECTIVE(TTimerCancelDirective, TStringBuf("timer_cancel"))
CREATE_AND_REGISTER_DIRECTIVE(TTimerPauseDirective, TStringBuf("timer_pause"))
CREATE_AND_REGISTER_DIRECTIVE(TTimerResumeDirective, TStringBuf("timer_resume"))
CREATE_AND_REGISTER_DIRECTIVE(TTimerSetDirective, TStringBuf("timer_set"))
CREATE_AND_REGISTER_DIRECTIVE(TTimerStopPlayingDirective, TStringBuf("timer_stop_playing"))
CREATE_AND_REGISTER_DIRECTIVE(TTimerSleepNowDirective, TStringBuf("timer_sleep_now"))

////////////////////////////////////////////////////////////////////////////////

// Translate
CREATE_AND_REGISTER_DIRECTIVE(TTranslateAsYandexDirective, TStringBuf("translate_yandex"))
CREATE_AND_REGISTER_DIRECTIVE(TTranslateAsGoogleDirective, TStringBuf("translate_google"))
CREATE_AND_REGISTER_DIRECTIVE(TTranslateAsUnknownDirective, TStringBuf("translate_unknown"))
CREATE_AND_REGISTER_DIRECTIVE(TTranslateSearchDirective, TStringBuf("translate_search"))

////////////////////////////////////////////////////////////////////////////////

// TTS
CREATE_AND_REGISTER_DIRECTIVE(TTtsPlayPlaceholderDirective, TStringBuf("tts_play_placeholder"))

////////////////////////////////////////////////////////////////////////////////

// Serp Gallery
CREATE_AND_REGISTER_DIRECTIVE(TSerpGalleryCallDirective, TStringBuf("serp_gallery_call"))
CREATE_AND_REGISTER_DIRECTIVE(TSerpGalleryOpenDirective, TStringBuf("serp_gallery_open"))

////////////////////////////////////////////////////////////////////////////////

// Market
CREATE_AND_REGISTER_DIRECTIVE(TMarketAddAndOpenCart, TStringBuf("market_add_and_open_cart"))

////////////////////////////////////////////////////////////////////////////////

// Mordovia (YandexStation WebView)
CREATE_AND_REGISTER_DIRECTIVE(TMordoviaShow, TStringBuf("mordovia_show"))
CREATE_AND_REGISTER_DIRECTIVE(TMordoviaCommand, TStringBuf("mordovia_command"))

////////////////////////////////////////////////////////////////////////////////

// Read Page Aloud
CREATE_AND_REGISTER_DIRECTIVE(TBrowserReadPageDirective, TStringBuf("browser_read_page"))
CREATE_AND_REGISTER_DIRECTIVE(TBrowserReadPagePauseDirective, TStringBuf("browser_read_page_pause"))
CREATE_AND_REGISTER_DIRECTIVE(TBrowserReadPageContinueDirective, TStringBuf("browser_read_page_continue"))

////////////////////////////////////////////////////////////////////////////////

// Tv Set
CREATE_AND_REGISTER_DIRECTIVE(TTvSwitchInputDirective, TStringBuf("tv_set_switch_input"))

////////////////////////////////////////////////////////////////////////////////

// LED screen
CREATE_AND_REGISTER_DIRECTIVE(TDrawLedScreenDirective, TStringBuf("draw_led_screen"))
CREATE_AND_REGISTER_DIRECTIVE(TForceDisplayCardsDirective, TStringBuf("force_display_cards"))

////////////////////////////////////////////////////////////////////////////////

// Push and Personal Cards
CREATE_AND_REGISTER_DIRECTIVE(TSendPushDirective, TStringBuf("send_push"))
// deprecated
CREATE_AND_REGISTER_DIRECTIVE(TPersonalCardsDirective, TStringBuf("personal_cards"))
CREATE_AND_REGISTER_DIRECTIVE(TPushMessageDirective, TStringBuf("push_message"))

////////////////////////////////////////////////////////////////////////////////

// TV/Module thin directives
CREATE_AND_REGISTER_DIRECTIVE(TTvOpenSearchScreenDirective, TStringBuf("tv_open_search_screen"))
CREATE_AND_REGISTER_DIRECTIVE(TTvOpenDetailsScreenDirective, TStringBuf("tv_open_details_screen"))
CREATE_AND_REGISTER_DIRECTIVE(TTvOpenPersonScreenDirective, TStringBuf("tv_open_person_screen"))
CREATE_AND_REGISTER_DIRECTIVE(TTvOpenCollectionScreenDirective, TStringBuf("tv_open_collection_screen"))
CREATE_AND_REGISTER_DIRECTIVE(TTvOpenSeriesScreenDirective, TStringBuf("tv_open_series_screen"))

////////////////////////////////////////////////////////////////////////////////

// Common media directives
CREATE_AND_REGISTER_DIRECTIVE(TShowButtonsDirective, TStringBuf("show_buttons"))
CREATE_AND_REGISTER_DIRECTIVE(TFillCloudUiDirective, TStringBuf("fill_cloud_ui"))

////////////////////////////////////////////////////////////////////////////////

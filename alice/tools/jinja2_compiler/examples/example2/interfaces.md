# Документация по TInterfaces.proto

Это пример документации, которая может быть подготовлена на основании протобаф описания
## Поле HasScreen 

Тип поля: bool, ассоциированное название фичи: ``

Комментарии из исходного протофайла:
```cpp
// True if the app can display the response from Alice (as opposed to
//  smart speakers, for example).
//  Relies on data from the app. Please, look for alternatives if possible.
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле VoiceSession 

Тип поля: bool, ассоциированное название фичи: ``

Комментарии из исходного протофайла:
```cpp
// True if the session started with voice activation (as opposed to typing).
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле HasReliableSpeakers 

Тип поля: bool, ассоциированное название фичи: `no_reliable_speakers`

Комментарии из исходного протофайла:
```cpp
// True if the app has access to speakers (false for Module, for example).
//  The flag is not reliable due to wrong default value.
// (BASS: !no_reliable_speakers) динамики "надежные" (используется для напоминаний и будильников)
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то возврат false
## Поле HasBluetooth 

Тип поля: bool, ассоциированное название фичи: ``

Комментарии из исходного протофайла:
```cpp
// True if the app has access to Bluetooth.
// not reliable due to wrong default value
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле HasAccessToBatteryPowerState 

Тип поля: bool, ассоциированное название фичи: `battery_power_state`

Комментарии из исходного протофайла:
```cpp
// True if the app has access to the state of the battery on the device.
// клиент может имеет доступ к заряду батареи
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то проверка среди unsupported features (возврат false, если есть)
* Если фичи нет в обоих списках, по умолчанию возвращается false
## Поле HasCEC 

Тип поля: bool, ассоциированное название фичи: `cec_available`

Комментарии из исходного протофайла:
```cpp
// True if the app can turn off a screen connected to the device through
//  HDMI.
// клиент может выключать экран через HDMI
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то проверка среди unsupported features (возврат false, если есть)
* Если фичи нет в обоих списках, по умолчанию возвращается false
## Поле CanChangeAlarmSound 

Тип поля: bool, ассоциированное название фичи: `change_alarm_sound`

Комментарии из исходного протофайла:
```cpp
// True if the app can change the sound that the device uses for alarms.
// клиент поддерживает изменение звука будильника
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то проверка среди unsupported features (возврат false, если есть)
* Если фичи нет в обоих списках, по умолчанию возвращается false
## Поле HasMicrophone 

Тип поля: bool, ассоциированное название фичи: ``

Комментарии из исходного протофайла:
```cpp
// True if the app has access to the microphone on the device.
// not reliable due to wrong default value
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле HasMusicPlayerShots 

Тип поля: bool, ассоциированное название фичи: ``

Комментарии из исходного протофайла:
```cpp
// The device firmware supports Music "shots".
//  TODO: clarify with Music
// клиент (музыкальный плеер) поддерживает шоты
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле IsTvPlugged 

Тип поля: bool, ассоциированное название фичи: ``

Комментарии из исходного протофайла:
```cpp
// True if there's a TV currently plugged into the device.
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле CanSetAlarm 

Тип поля: bool, ассоциированное название фичи: `set_alarm`

Комментарии из исходного протофайла:
```cpp
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле CanSetTimer 

Тип поля: bool, ассоциированное название фичи: `set_timer`

Комментарии из исходного протофайла:
```cpp
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле CanOpenLink 

Тип поля: bool, ассоциированное название фичи: `open_link`

Комментарии из исходного протофайла:
```cpp
// client can open link in internal browser
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле CanOpenLinkTurboApp 

Тип поля: bool, ассоциированное название фичи: `open_link_turboapp`

Комментарии из исходного протофайла:
```cpp
// client can open turbo app
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле HasSynchronizedPush 

Тип поля: bool, ассоциированное название фичи: ``

Комментарии из исходного протофайла:
```cpp
// client implements pushes without races
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле SupportsVideoProtocol 

Тип поля: bool, ассоциированное название фичи: `video_protocol`

Комментарии из исходного протофайла:
```cpp
// client supports video protocol (knows how to handle video directives)
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле CanShowGif 

Тип поля: bool, ассоциированное название фичи: ``

Комментарии из исходного протофайла:
```cpp
// subset of clients supporting div2 cards
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле HasLedDisplay 

Тип поля: bool, ассоциированное название фичи: `led_display`

Комментарии из исходного протофайла:
```cpp
// client supports led display to show animation
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то проверка среди unsupported features (возврат false, если есть)
* Если фичи нет в обоих списках, по умолчанию возвращается false
## Поле TtsPlayPlaceholder 

Тип поля: bool, ассоциированное название фичи: `tts_play_placeholder`

Комментарии из исходного протофайла:
```cpp
// client can voice tts (tts_play_placeholder directive) in arbitrary place among directives
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то проверка среди unsupported features (возврат false, если есть)
* Если фичи нет в обоих списках, по умолчанию возвращается false
## Поле Multiroom 

Тип поля: bool, ассоциированное название фичи: `multiroom`

Комментарии из исходного протофайла:
```cpp
// True if device has multiroom support
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то возврат false
## Поле HasAudioClient 

Тип поля: bool, ассоциированное название фичи: `audio_client`

Комментарии из исходного протофайла:
```cpp
// client supports thin audio player
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то проверка среди unsupported features (возврат false, если есть)
* Если фичи нет в обоих списках, по умолчанию возвращается false
## Поле IsPubliclyAvailable 

Тип поля: bool, ассоциированное название фичи: `publicly_available`

Комментарии из исходного протофайла:
```cpp
// client is publicly available (risky client due to some legal reasons)
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то проверка среди unsupported features (возврат false, если есть)
* Если фичи нет в обоих списках, по умолчанию возвращается false
## Поле HasNotifications 

Тип поля: bool, ассоциированное название фичи: `notifications`

Комментарии из исходного протофайла:
```cpp
// client supports push notifications via alice notificator
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то проверка среди unsupported features (возврат false, если есть)
* Если фичи нет в обоих списках, по умолчанию возвращается false
## Поле HasBluetoothPlayer 

Тип поля: bool, ассоциированное название фичи: `bluetooth_player`

Комментарии из исходного протофайла:
```cpp
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то проверка среди unsupported features (возврат false, если есть)
* Если фичи нет в обоих списках, по умолчанию возвращается false
## Поле CanChangeAlarmSoundLevel 

Тип поля: bool, ассоциированное название фичи: `change_alarm_sound_level`

Комментарии из исходного протофайла:
```cpp
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то проверка среди unsupported features (возврат false, если есть)
* Если фичи нет в обоих списках, по умолчанию возвращается false
## Поле CanServerAction 

Тип поля: bool, ассоциированное название фичи: `server_action`

Комментарии из исходного протофайла:
```cpp
// client implements server actions (special alice backend requests)
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле CanRecognizeMusic 

Тип поля: bool, ассоциированное название фичи: `music_recognizer`

Комментарии из исходного протофайла:
```cpp
// client supports music recognition with music request
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле HasMordoviaWebView 

Тип поля: bool, ассоциированное название фичи: `mordovia_webview`

Комментарии из исходного протофайла:
```cpp
// client supports mordovia directives
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле IncomingMessengerCalls 

Тип поля: bool, ассоциированное название фичи: `incoming_messenger_calls`

Комментарии из исходного протофайла:
```cpp
// client supports incoming messenger calls
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то возврат false
## Поле SupportsBluetoothRCU 

Тип поля: bool, ассоциированное название фичи: `bluetooth_rcu`

Комментарии из исходного протофайла:
```cpp
// client supports pairing to a remote control unit via bluetooth
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то проверка среди unsupported features (возврат false, если есть)
* Если фичи нет в обоих списках, по умолчанию возвращается false
## Поле LiveTvScheme 

Тип поля: bool, ассоциированное название фичи: `live_tv_scheme`

Комментарии из исходного протофайла:
```cpp
// True if client supports live-tv scheme urls (publishes own url list based on internal channel db state
//  to backend and knows how to handle such urls after receiving)
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то возврат false
## Поле CanOpenQuasarScreen 

Тип поля: bool, ассоциированное название фичи: `quasar_screen`

Комментарии из исходного протофайла:
```cpp
// client can open quasar screen
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле VideoCodecAVC 

Тип поля: bool, ассоциированное название фичи: `video_codec_AVC`

Комментарии из исходного протофайла:
```cpp
// Video codecs
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то возврат false
## Поле VideoCodecHEVC 

Тип поля: bool, ассоциированное название фичи: `video_codec_HEVC`

Комментарии из исходного протофайла:
```cpp
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то возврат false
## Поле VideoCodecVP9 

Тип поля: bool, ассоциированное название фичи: `video_codec_VP9`

Комментарии из исходного протофайла:
```cpp
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то возврат false
## Поле AudioCodecDD (устарело)

Тип поля: bool, ассоциированное название фичи: ``

Комментарии из исходного протофайла:
```cpp
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле AudioCodecDTS (устарело)

Тип поля: bool, ассоциированное название фичи: ``

Комментарии из исходного протофайла:
```cpp
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле AudioCodecAAC 

Тип поля: bool, ассоциированное название фичи: `audio_codec_AAC`

Комментарии из исходного протофайла:
```cpp
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то возврат false
## Поле AudioCodecEC3 (устарело)

Тип поля: bool, ассоциированное название фичи: ``

Комментарии из исходного протофайла:
```cpp
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле CurrentHDCPLevelNone 

Тип поля: bool, ассоциированное название фичи: ``

Комментарии из исходного протофайла:
```cpp
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле CurrentHDCPLevel1X 

Тип поля: bool, ассоциированное название фичи: ``

Комментарии из исходного протофайла:
```cpp
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле CurrentHDCPLevel2X 

Тип поля: bool, ассоциированное название фичи: ``

Комментарии из исходного протофайла:
```cpp
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле DynamicRangeSDR 

Тип поля: bool, ассоциированное название фичи: `dynamic_range_SDR`

Комментарии из исходного протофайла:
```cpp
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то возврат false
## Поле DynamicRangeHDR10 

Тип поля: bool, ассоциированное название фичи: `dynamic_range_HDR10`

Комментарии из исходного протофайла:
```cpp
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то возврат false
## Поле DynamicRangeHDR10Plus 

Тип поля: bool, ассоциированное название фичи: `dynamic_range_HDR10Plus`

Комментарии из исходного протофайла:
```cpp
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то возврат false
## Поле DynamicRangeDV 

Тип поля: bool, ассоциированное название фичи: `dynamic_range_DV`

Комментарии из исходного протофайла:
```cpp
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то возврат false
## Поле DynamicRangeHLG 

Тип поля: bool, ассоциированное название фичи: `dynamic_range_HLG`

Комментарии из исходного протофайла:
```cpp
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то возврат false
## Поле VideoFormatSD 

Тип поля: bool, ассоциированное название фичи: ``

Комментарии из исходного протофайла:
```cpp
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле VideoFormatHD 

Тип поля: bool, ассоциированное название фичи: ``

Комментарии из исходного протофайла:
```cpp
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле VideoFormatUHD 

Тип поля: bool, ассоциированное название фичи: ``

Комментарии из исходного протофайла:
```cpp
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле CanOpenWhocalls 

Тип поля: bool, ассоциированное название фичи: `whocalls`

Комментарии из исходного протофайла:
```cpp
// client can open whocalls onboarding
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле AudioCodecEAC3 

Тип поля: bool, ассоциированное название фичи: `audio_codec_EAC3`

Комментарии из исходного протофайла:
```cpp
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то возврат false
## Поле AudioCodecAC3 

Тип поля: bool, ассоциированное название фичи: `audio_codec_AC3`

Комментарии из исходного протофайла:
```cpp
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то возврат false
## Поле AudioCodecVORBIS 

Тип поля: bool, ассоциированное название фичи: `audio_codec_VORBIS`

Комментарии из исходного протофайла:
```cpp
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то возврат false
## Поле AudioCodecOPUS 

Тип поля: bool, ассоциированное название фичи: `audio_codec_OPUS`

Комментарии из исходного протофайла:
```cpp
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то возврат false
## Поле HasMusicSdkClient 

Тип поля: bool, ассоциированное название фичи: `music_sdk_client`

Комментарии из исходного протофайла:
```cpp
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле CanRecognizeImage 

Тип поля: bool, ассоциированное название фичи: `image_recognizer`

Комментарии из исходного протофайла:
```cpp
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле CanRenderDivCards 

Тип поля: bool, ассоциированное название фичи: `div_cards`

Комментарии из исходного протофайла:
```cpp
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле CanOpenLinkIntent 

Тип поля: bool, ассоциированное название фичи: `open_link_intent`

Комментарии из исходного протофайла:
```cpp
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле MultiroomCluster 

Тип поля: bool, ассоциированное название фичи: `multiroom_cluster`

Комментарии из исходного протофайла:
```cpp
// True if device has multiroom cluster support
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то возврат false
## Поле CanRenderDiv2Cards 

Тип поля: bool, ассоциированное название фичи: `div2_cards`

Комментарии из исходного протофайла:
```cpp
// Actually tells that this is PP that can render div2 card from response.layout.card; Consider as Deprecated in Clients other than PP
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле CanOpenDialogsInTabs 

Тип поля: bool, ассоциированное название фичи: `open_dialogs_in_tabs`

Комментарии из исходного протофайла:
```cpp
// client can open external skills in separate tabs
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле CanOpenLinkSearchViewport 

Тип поля: bool, ассоциированное название фичи: `open_link_search_viewport`

Комментарии из исходного протофайла:
```cpp
// client can open serp with viewport:// url
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле HasDirectiveSequencer 

Тип поля: bool, ассоциированное название фичи: `directive_sequencer`

Комментарии из исходного протофайла:
```cpp
// client supports sequential directive execution
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то проверка среди unsupported features (возврат false, если есть)
* Если фичи нет в обоих списках, по умолчанию возвращается false
## Поле CanOpenKeyboard 

Тип поля: bool, ассоциированное название фичи: `keyboard`

Комментарии из исходного протофайла:
```cpp
// client can open keyboard onboarding/settings
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то проверка среди unsupported features (возврат false, если есть)
* Если фичи нет в обоих списках, по умолчанию возвращается false
## Поле HasCloudPush 

Тип поля: bool, ассоциированное название фичи: ``

Комментарии из исходного протофайла:
```cpp
// client implements cloud-based pushes
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле CanOpenYandexAuth 

Тип поля: bool, ассоциированное название фичи: `open_yandex_auth`

Комментарии из исходного протофайла:
```cpp
// True if device can open "yandex-auth://" links for authorization
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле HasMusicQuasarClient 

Тип поля: bool, ассоциированное название фичи: `music_quasar_client`

Комментарии из исходного протофайла:
```cpp
// client supports legacy quasar music player
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то проверка среди unsupported features (возврат false, если есть)
* Если фичи нет в обоих списках, по умолчанию возвращается false
## Поле CanSetAlarmSemanticFrame 

Тип поля: bool, ассоциированное название фичи: ``

Комментарии из исходного протофайла:
```cpp
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле HasNavigator 

Тип поля: bool, ассоциированное название фичи: `navigator`

Комментарии из исходного протофайла:
```cpp
// supports navigator protocol
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле CanOpenLinkYellowskin 

Тип поля: bool, ассоциированное название фичи: `open_link_yellowskin`

Комментарии из исходного протофайла:
```cpp
// client supports yellowskin js-api
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле SupportsShowView 

Тип поля: bool, ассоциированное название фичи: `show_view`

Комментарии из исходного протофайла:
```cpp
// client supports show_view directive
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то проверка среди unsupported features (возврат false, если есть)
* Если фичи нет в обоих списках, по умолчанию возвращается false
## Поле SupportsFMRadio 

Тип поля: bool, ассоциированное название фичи: ``

Комментарии из исходного протофайла:
```cpp
// client supports listening to FM radio
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле OutgoingPhoneCalls 

Тип поля: bool, ассоциированное название фичи: `outgoing_phone_calls`

Комментарии из исходного протофайла:
```cpp
// client supports outgoing phone calls
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле CanOpenIBroSettings 

Тип поля: bool, ассоциированное название фичи: `open_ibro_settings`

Комментарии из исходного протофайла:
```cpp
// client can open system browser settings screen with custom push
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то проверка среди unsupported features (возврат false, если есть)
* Если фичи нет в обоих списках, по умолчанию возвращается false
## Поле SupportsHDMIOutput 

Тип поля: bool, ассоциированное название фичи: ``

Комментарии из исходного протофайла:
```cpp
// client supports streaming video data through hdmi
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле SupportsAudioBitrate192Kbps 

Тип поля: bool, ассоциированное название фичи: `audio_bitrate192`

Комментарии из исходного протофайла:
```cpp
// client's audio player supports middle-level sound bitrate
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то возврат false
## Поле SupportsAudioBitrate320Kbps 

Тип поля: bool, ассоциированное название фичи: `audio_bitrate320`

Комментарии из исходного протофайла:
```cpp
// client's audio player supports the highest sound bitrate
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то возврат false
## Поле SupportsVideoPlayDirective 

Тип поля: bool, ассоциированное название фичи: `video_play_directive`

Комментарии из исходного протофайла:
```cpp
// client supports video_play directive
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле CanOpenReader 

Тип поля: bool, ассоциированное название фичи: ``

Комментарии из исходного протофайла:
```cpp
// client can open reader app
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле SupportsUnauthorizedMusicDirectives 

Тип поля: bool, ассоциированное название фичи: `unauthorized_music_directives`

Комментарии из исходного протофайла:
```cpp
// client supports music directives for unauthorized user
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то проверка среди unsupported features (возврат false, если есть)
* Если фичи нет в обоих списках, по умолчанию возвращается false
## Поле SupportsTvOpenCollectionScreenDirective 

Тип поля: bool, ассоциированное название фичи: `tv_open_collection_screen_directive`

Комментарии из исходного протофайла:
```cpp
// client supports tv_open_collection_screen directive
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то проверка среди unsupported features (возврат false, если есть)
* Если фичи нет в обоих списках, по умолчанию возвращается false
## Поле SupportsTvOpenDetailsScreenDirective 

Тип поля: bool, ассоциированное название фичи: `tv_open_details_screen_directive`

Комментарии из исходного протофайла:
```cpp
// client supports tv_open_details_screen directive
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то проверка среди unsupported features (возврат false, если есть)
* Если фичи нет в обоих списках, по умолчанию возвращается false
## Поле SupportsTvOpenPersonScreenDirective 

Тип поля: bool, ассоциированное название фичи: `tv_open_person_screen_directive`

Комментарии из исходного протофайла:
```cpp
// client supports tv_open_person_screen directive
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то проверка среди unsupported features (возврат false, если есть)
* Если фичи нет в обоих списках, по умолчанию возвращается false
## Поле SupportsTvOpenSearchScreenDirective 

Тип поля: bool, ассоциированное название фичи: `tv_open_search_screen_directive`

Комментарии из исходного протофайла:
```cpp
// client supports tv_open_search_screen directive
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то проверка среди unsupported features (возврат false, если есть)
* Если фичи нет в обоих списках, по умолчанию возвращается false
## Поле SupportsTvOpenSeriesScreenDirective 

Тип поля: bool, ассоциированное название фичи: `tv_open_series_screen_directive`

Комментарии из исходного протофайла:
```cpp
// client supports tv_open_series_screen directive
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то проверка среди unsupported features (возврат false, если есть)
* Если фичи нет в обоих списках, по умолчанию возвращается false
## Поле SupportsCloudUi 

Тип поля: bool, ассоциированное название фичи: `cloud_ui`

Комментарии из исходного протофайла:
```cpp
// client supports Alice-cloud https://st.yandex-team.ru/IBRO-24652
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то проверка среди unsupported features (возврат false, если есть)
* Если фичи нет в обоих списках, по умолчанию возвращается false
## Поле MultiroomAudioClient 

Тип поля: bool, ассоциированное название фичи: `multiroom_audio_client`

Комментарии из исходного протофайла:
```cpp
// client supports multiroom on audio client
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то возврат false
## Поле SupportsDivCardsRendering 

Тип поля: bool, ассоциированное название фичи: ``

Комментарии из исходного протофайла:
```cpp
// proxy interface for client support one of div cards (CanRenderDivCards or CanRenderDiv2Cards)
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле CanOpenCovidQrCode 

Тип поля: bool, ассоциированное название фичи: `covid_qr`

Комментарии из исходного протофайла:
```cpp
// can open covid qr code in app
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле HasAudioClientHls 

Тип поля: bool, ассоциированное название фичи: `audio_client_hls`

Комментарии из исходного протофайла:
```cpp
// client supports hls in thin audio player
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то проверка среди unsupported features (возврат false, если есть)
* Если фичи нет в обоих списках, по умолчанию возвращается false
## Поле CanOpenPedometer 

Тип поля: bool, ассоциированное название фичи: `pedometer`

Комментарии из исходного протофайла:
```cpp
// client can open pedometer app
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то возврат false
## Поле SupportsVerticalScreenNavigation 

Тип поля: bool, ассоциированное название фичи: `vertical_screen_navigation`

Комментарии из исходного протофайла:
```cpp
// client supports vertical screen navigation directives (go_up/go_down)
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле CanOpenWhocallsBlocking 

Тип поля: bool, ассоциированное название фичи: `whocalls_call_blocking`

Комментарии из исходного протофайла:
```cpp
// client can open whocalls blocking settings
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то возврат false
## Поле SupportsMapsDownloadOffline 

Тип поля: bool, ассоциированное название фичи: `maps_download_offline`

Комментарии из исходного протофайла:
```cpp
// client can download offline maps https://st.yandex-team.ru/MAPSPRODUCT-1623
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то проверка среди unsupported features (возврат false, если есть)
* Если фичи нет в обоих списках, по умолчанию возвращается false
## Поле CanOpenPasswordManager 

Тип поля: bool, ассоциированное название фичи: ``

Комментарии из исходного протофайла:
```cpp
// client can open password manager via deeplink
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле SupportsGoHomeDirective 

Тип поля: bool, ассоциированное название фичи: `go_home`

Комментарии из исходного протофайла:
```cpp
// // client supports go_home directive
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то проверка среди unsupported features (возврат false, если есть)
* Если фичи нет в обоих списках, по умолчанию возвращается false
## Поле CanOpenBonusCardsCamera 

Тип поля: bool, ассоциированное название фичи: `bonus_cards_camera`

Комментарии из исходного протофайла:
```cpp
// client supports smart camera in bonus cards mode
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то возврат false
## Поле CanOpenBonusCardsList 

Тип поля: bool, ассоциированное название фичи: `bonus_cards_list`

Комментарии из исходного протофайла:
```cpp
// client can open bonus cards list
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то возврат false
## Поле CanShowTimer 

Тип поля: bool, ассоциированное название фичи: `show_timer`

Комментарии из исходного протофайла:
```cpp
// client supports timer_show directive
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле HasMusicPlayer 

Тип поля: bool, ассоциированное название фичи: ``

Комментарии из исходного протофайла:
```cpp
// client has internal music player
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле SupportsDeviceLocalReminders 

Тип поля: bool, ассоциированное название фичи: `supports_device_local_reminders`

Комментарии из исходного протофайла:
```cpp
// client supports device local reminders
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то возврат false
## Поле CanOpenWhocallsMessageFiltering 

Тип поля: bool, ассоциированное название фичи: `whocalls_message_filtering`

Комментарии из исходного протофайла:
```cpp
// client can open message filtering
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то возврат false
## Поле HasScledDisplay 

Тип поля: bool, ассоциированное название фичи: `scled_display`

Комментарии из исходного протофайла:
```cpp
// client supports 7-segment-led display to show animation (Mini 2)
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то возврат false
## Поле PhoneAddressBook 

Тип поля: bool, ассоциированное название фичи: `phone_address_book`

Комментарии из исходного протофайла:
```cpp
// client can send the address book
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то возврат false
## Поле SupportsAbsoluteVolumeChange 

Тип поля: bool, ассоциированное название фичи: `absolute_volume_change`

Комментарии из исходного протофайла:
```cpp
// client supports sound_set_level directive
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то проверка среди unsupported features (возврат false, если есть)
* Если фичи нет в обоих списках, по умолчанию возвращается true
## Поле SupportsRelativeVolumeChange 

Тип поля: bool, ассоциированное название фичи: `relative_volume_change`

Комментарии из исходного протофайла:
```cpp
// client supports sound_louder/sound_quiter directive
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то проверка среди unsupported features (возврат false, если есть)
* Если фичи нет в обоих списках, по умолчанию возвращается false
## Поле CanOpenVideotranslationOnboarding 

Тип поля: bool, ассоциированное название фичи: ``

Комментарии из исходного протофайла:
```cpp
// client can open videotranslation onboarding
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле OpenAddressBook 

Тип поля: bool, ассоциированное название фичи: `open_address_book`

Комментарии из исходного протофайла:
```cpp
// client can open the address book
```

Логика обработки в коде:
* Кастомная логика обработки
## Поле HasClockDisplay 

Тип поля: bool, ассоциированное название фичи: `clock_display`

Комментарии из исходного протофайла:
```cpp
// client shows clock on its display
```

Логика обработки в коде:
* Проверка среди supported features (возврат true, если есть)
* Если отсутствует, то проверка среди unsupported features (возврат false, если есть)
* Если фичи нет в обоих списках, по умолчанию возвращается false

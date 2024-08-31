# List of lstm-based binary intent classifiers with PREFIX `lstm_models/<intent name>`
FROM_SANDBOX(
    2211955711
    PREFIX
    ${PREFIX}lstm_models/alice.ontofacts
    OUT
    ${PREFIX}lstm_models/alice.ontofacts/model.pb
    ${PREFIX}lstm_models/alice.ontofacts/model_description.json
)

FROM_SANDBOX(
    2410964635
    RENAME
    show_route_lstm/model.pb
    show_route_lstm/model_description.json
    OUT
    ${PREFIX}lstm_models/personal_assistant.scenarios.show_route/model.pb
    ${PREFIX}lstm_models/personal_assistant.scenarios.show_route/model_description.json
)

FROM_SANDBOX(
    2579577603
    RENAME
    market_lstm/model.pb
    market_lstm/model_description.json
    OUT
    ${PREFIX}lstm_models/alice.market.how_much.lstm_like_alice/model.pb
    ${PREFIX}lstm_models/alice.market.how_much.lstm_like_alice/model_description.json
)

# List of dssm-based binary intent classifiers with PREFIX `dssm_models/<intent name>`
FROM_SANDBOX(
    1430645507
    PREFIX
    ${PREFIX}dssm_models/personal_assistant.scenarios.music_play
    OUT
    ${PREFIX}dssm_models/personal_assistant.scenarios.music_play/model.pb
    ${PREFIX}dssm_models/personal_assistant.scenarios.music_play/model_description.json
)

FROM_SANDBOX(
    1821640101
    PREFIX
    ${PREFIX}dssm_models/personal_assistant.scenarios.get_news
    OUT
    ${PREFIX}dssm_models/personal_assistant.scenarios.get_news/model.pb
    ${PREFIX}dssm_models/personal_assistant.scenarios.get_news/model_description.json
)

FROM_SANDBOX(
    2479324864
    RENAME
    model/model.pb
    model/model_description.json
    OUT
    ${PREFIX}dssm_models/alice.market.how_much/model.pb
    ${PREFIX}dssm_models/alice.market.how_much/model_description.json
)

FROM_SANDBOX(
    2853648486
    RENAME
    model/model.pb
    model/model_description.json
    OUT
    ${PREFIX}dssm_models/alice.apps_fixlist.muzpult.dssm/model.pb
    ${PREFIX}dssm_models/alice.apps_fixlist.muzpult.dssm/model_description.json
)

FROM_SANDBOX(
    2505381910
    RENAME
    fixlist_pdd_beggins/model.pb
    fixlist_pdd_beggins/model_description.json
    OUT
    ${PREFIX}beggins_models/AliceBegginsFixlistPdd/model.pb
    ${PREFIX}beggins_models/AliceBegginsFixlistPdd/model_description.json
)

FROM_SANDBOX(
    2656324343
    RENAME
    turn_clock_face_on_extended/model.cbm
    OUT_NOAUTO
    ${PREFIX}beggins_models/AliceBegginsTurnClockFaceOn/model.cbm
)

FROM_SANDBOX(
    2656323278
    RENAME
    turn_clock_face_off_extended/model.cbm
    OUT_NOAUTO
    ${PREFIX}beggins_models/AliceBegginsTurnClockFaceOff/model.cbm
)

FROM_SANDBOX(
    2726858031
    RENAME
    qr_code/model.cbm
    OUT_NOAUTO
    ${PREFIX}beggins_models/AliceBegginsFixlistQRCode/model.cbm
)

FROM_SANDBOX(
    3005073306
    RENAME
    rent_apartment/model.cbm
    OUT_NOAUTO
    ${PREFIX}beggins_models/AliceBegginsRentApartmentAsOwner/model.cbm
)

FROM_SANDBOX(
    2848646376
    RENAME
    zeliboba_qr_code/model.cbm
    OUT_NOAUTO
    ${PREFIX}zeliboba_models/AliceZelibobaQRCode/model.cbm
)

FROM_SANDBOX(
    3285174249
    RENAME
    set_my_name/model.cbm
    OUT_NOAUTO
    ${PREFIX}zeliboba_models/AliceZelibobaSetMyName/model.cbm
)

FROM_SANDBOX(
    2765261214
    RENAME
    fines/model.cbm
    OUT_NOAUTO
    ${PREFIX}beggins_models/AliceBegginsFixlistFines/model.cbm
)

FROM_SANDBOX(
    2729377092
    RENAME
    model/model.pb
    model/model_description.json
    OUT
    ${PREFIX}mixed_models/alice.market.how_much.dssm_like_alice/model.pb
    ${PREFIX}mixed_models/alice.market.how_much.dssm_like_alice/model_description.json
)

FROM_SANDBOX(
    2769956096
    RENAME
    read_page/model.cbm
    OUT_NOAUTO
    ${PREFIX}beggins_models/AliceBegginsFixlistReadPage/model.cbm
)

FROM_SANDBOX(
    2778271067
    RENAME
    pdd/model.cbm
    OUT_NOAUTO
    ${PREFIX}beggins_models/AliceBegginsFixlistReceipts/model.cbm
)

FROM_SANDBOX(
    2786289036
    RENAME
    goods_reask/model.cbm
    OUT_NOAUTO
    ${PREFIX}beggins_models/AliceBegginsGoodsBestPricesReask/model.cbm
)

FROM_SANDBOX(
    3128042942
    RENAME
    what_is_my_name/model.cbm
    OUT_NOAUTO
    ${PREFIX}zeliboba_models/AliceZelibobaWhatIsMyName/model.cbm
)

FROM_SANDBOX(
    3064324576
    RENAME
    metronome_faster/model.cbm
    OUT_NOAUTO
    ${PREFIX}beggins_models/AliceBegginsMetronomeFaster/model.cbm
)

FROM_SANDBOX(
    3061026734
    RENAME
    metronome_start/model.cbm
    OUT_NOAUTO
    ${PREFIX}beggins_models/AliceBegginsMetronomeStart/model.cbm
)

FROM_SANDBOX(
    3064393328
    RENAME
    metronome_slower/model.cbm
    OUT_NOAUTO
    ${PREFIX}beggins_models/AliceBegginsMetronomeSlower/model.cbm
)

FROM_SANDBOX(
    2799765470
    RENAME
    keyboard/model.cbm
    OUT_NOAUTO
    ${PREFIX}beggins_models/AliceBegginsFixlistKeyboard/model.cbm
)

FROM_SANDBOX(
    2924735536
    RENAME
    ege/model.cbm
    OUT_NOAUTO
    ${PREFIX}beggins_models/AliceBegginsEge/model.cbm
)

FROM_SANDBOX(
    2991585326
    RENAME
    open_ya_rent/model.cbm
    OUT_NOAUTO
    ${PREFIX}beggins_models/AliceBegginsOpenYaRent/model.cbm
)

FROM_SANDBOX(
    3009112732
    RENAME
    open_ya_rent/model.cbm
    OUT_NOAUTO
    ${PREFIX}beggins_models/AliceBegginsRentApartmentAsTenant/model.cbm
)

FROM_SANDBOX(
    3351860100
    RENAME
    open_screensaver/model.cbm
    OUT_NOAUTO
    ${PREFIX}zeliboba_models/AliceZelibobaOpenScreensaver/model.cbm
)

FROM_SANDBOX(
    3199714567
    RENAME
    tv_secret_promo/model.cbm
    OUT_NOAUTO
    ${PREFIX}zeliboba_models/AliceZelibobaTvSecretPromo/model.cbm
)

FROM_SANDBOX(
    3269341396
    RENAME
    voiceprint_remove/model.cbm
    OUT_NOAUTO
    ${PREFIX}zeliboba_models/AliceZelibobaVoiceprintRemove/model.cbm
)

FROM_SANDBOX(
    3074986273
    RENAME
    yandex_profile_documents/model.cbm
    OUT_NOAUTO
    ${PREFIX}zeliboba_models/AliceZelibobaYandexProfileDocuments/model.cbm
)

FROM_SANDBOX(
    3265029931
    RENAME
    guru/model.cbm
    OUT_NOAUTO
    ${PREFIX}zeliboba_models/AliceZelibobaAdvancedGuru/model.cbm
)

FROM_SANDBOX(
    3355995911
    RENAME
    video_call_mute_mic/model.cbm
    OUT_NOAUTO
    ${PREFIX}zeliboba_models/AliceZelibobaVideoCallMuteMic/model.cbm
)

FROM_SANDBOX(
    3355999517
    RENAME
    video_call_unmute_mic/model.cbm
    OUT_NOAUTO
    ${PREFIX}zeliboba_models/AliceZelibobaVideoCallUnmuteMic/model.cbm
)

FROM_SANDBOX(
    3353723551
    RENAME
    what_is_playing/model.cbm
    OUT_NOAUTO
    ${PREFIX}zeliboba_models/AliceZelibobaWhatIsPlaying/model.cbm
)

SET(
    MODEL_FILES
    ${PREFIX}lstm_models/alice.ontofacts/model.pb
    ${PREFIX}lstm_models/alice.ontofacts/model_description.json
    ${PREFIX}lstm_models/personal_assistant.scenarios.show_route/model.pb
    ${PREFIX}lstm_models/personal_assistant.scenarios.show_route/model_description.json
    ${PREFIX}lstm_models/alice.market.how_much.lstm_like_alice/model.pb
    ${PREFIX}lstm_models/alice.market.how_much.lstm_like_alice/model_description.json
    ${PREFIX}dssm_models/personal_assistant.scenarios.music_play/model.pb
    ${PREFIX}dssm_models/personal_assistant.scenarios.music_play/model_description.json
    ${PREFIX}dssm_models/personal_assistant.scenarios.get_news/model.pb
    ${PREFIX}dssm_models/personal_assistant.scenarios.get_news/model_description.json
    ${PREFIX}dssm_models/alice.market.how_much/model.pb
    ${PREFIX}dssm_models/alice.market.how_much/model_description.json
    ${PREFIX}dssm_models/alice.apps_fixlist.muzpult.dssm/model.pb
    ${PREFIX}dssm_models/alice.apps_fixlist.muzpult.dssm/model_description.json
    ${PREFIX}beggins_models/AliceBegginsFixlistPdd/model.pb
    ${PREFIX}beggins_models/AliceBegginsFixlistPdd/model_description.json
    ${PREFIX}beggins_models/AliceBegginsTurnClockFaceOn/model.cbm
    ${PREFIX}beggins_models/AliceBegginsTurnClockFaceOff/model.cbm
    ${PREFIX}beggins_models/AliceBegginsFixlistQRCode/model.cbm
    ${PREFIX}beggins_models/AliceBegginsFixlistFines/model.cbm
    ${PREFIX}mixed_models/alice.market.how_much.dssm_like_alice/model.pb
    ${PREFIX}mixed_models/alice.market.how_much.dssm_like_alice/model_description.json
    ${PREFIX}beggins_models/AliceBegginsFixlistReadPage/model.cbm
    ${PREFIX}beggins_models/AliceBegginsFixlistReceipts/model.cbm
    ${PREFIX}beggins_models/AliceBegginsGoodsBestPricesReask/model.cbm
    ${PREFIX}beggins_models/AliceBegginsMetronomeFaster/model.cbm
    ${PREFIX}beggins_models/AliceBegginsMetronomeSlower/model.cbm
    ${PREFIX}beggins_models/AliceBegginsMetronomeStart/model.cbm
    ${PREFIX}beggins_models/AliceBegginsFixlistKeyboard/model.cbm
    ${PREFIX}beggins_models/AliceBegginsRentApartmentAsOwner/model.cbm
    ${PREFIX}beggins_models/AliceBegginsEge/model.cbm
    ${PREFIX}zeliboba_models/AliceZelibobaQRCode/model.cbm
    ${PREFIX}zeliboba_models/AliceZelibobaSetMyName/model.cbm
    ${PREFIX}beggins_models/AliceBegginsOpenYaRent/model.cbm
    ${PREFIX}beggins_models/AliceBegginsRentApartmentAsTenant/model.cbm
    ${PREFIX}zeliboba_models/AliceZelibobaAdvancedGuru/model.cbm
    ${PREFIX}zeliboba_models/AliceZelibobaOpenScreensaver/model.cbm
    ${PREFIX}zeliboba_models/AliceZelibobaVoiceprintRemove/model.cbm
    ${PREFIX}zeliboba_models/AliceZelibobaWhatIsMyName/model.cbm
    ${PREFIX}zeliboba_models/AliceZelibobaYandexProfileDocuments/model.cbm
    ${PREFIX}zeliboba_models/AliceZelibobaVideoCallMuteMic/model.cbm
    ${PREFIX}zeliboba_models/AliceZelibobaVideoCallUnmuteMic/model.cbm
    ${PREFIX}zeliboba_models/AliceZelibobaWhatIsPlaying/model.cbm
)

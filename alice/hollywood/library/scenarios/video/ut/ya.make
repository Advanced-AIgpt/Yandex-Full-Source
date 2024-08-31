UNITTEST_FOR(alice/hollywood/library/scenarios/video)

OWNER(
    g:smarttv
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/framework/unittest
    alice/hollywood/library/testing
    apphost/lib/service_testing
    library/cpp/testing/gmock_in_unittest
)

SIZE(SMALL)

SRCS(
    analytics_ut.cpp
    parsers_ut.cpp
    video_dispatcher_ut.cpp
    card_detail_ut.cpp
    tv_actions_ut.cpp
)

DATA(
    sbr://2890592192=fixtures/ #  OTT документ с залогином film_ott_with_login.json
    sbr://2907338081=fixtures/ #  VH карусель c документами ut_vh_carousel.json
    sbr://2918311540=fixtures/ #  VH карусели ut_vh_carousels.json
    sbr://3007164483=fixtures/ #  Ответ видеопоиска на запрос "мультики" multiki.json
    sbr://3010630017=fixtures/ #  Ответ видеопоиска на запрос "игра престолов" game_of_thrones.json
    sbr://3013642488=fixtures/ #  cinemaData.json
    sbr://3127658612=fixtures/ #  Ответ видеопоиска на запрос "терминатор" с включенными film_offers: vh_exp_film_offers.json
    sbr://3162363678=fixtures/ #  вымышленные cinema_data : CinemaDataFaked.json
    sbr://3159886694=fixtures/ #  Ответ видеопоиска на запрос "плохие парни навсегда" с включенными film_offers: BBF.json
    sbr://3174887850=fixtures/ #  Ответ видеопиоска на запрос "терминатор 2" : terminator_2.json
    sbr://3181929551=fixtures/ #  Ответ видеопоиска на запрос "гарфилд" : garfild.json
    sbr://3190635873=fixtures/ #  Ответ droideka на card_detail "Исходный код" : card_detail_ww_false.json
    sbr://3244174409=fixtures/ #  Ответ вебного поиска на запрос "смычок" : Smychok.json
    sbr://3250737053=fixtures/ #  Ответ вебного поиска на запрос "призраки" : Prizraki.json
    sbr://3250738759=fixtures/ #  Ответ вебного поиска на запрос "как я встретил вашу маму": HowIMetYourMother.json
    sbr://3250485369=fixtures/ #  Ответ вебного поиска на запрос "сверхъестественное": Supernatural.json
    sbr://3254907184=fixtures/ #  Ответ вебного поиска на запрос "матрица" : Matrix.json
    sbr://3297631442=fixtures/ #  Ответ вебного поиска на запрос "друзья сериал": Friends.json
)

END()

PY3TEST()

OWNER(
    g:smarttv
)

SIZE(MEDIUM)

FORK_SUBTESTS()
SPLIT_FACTOR(4)
SET(HOLLYWOOD_SHARD video)
ENV(HOLLYWOOD_SHARD=video)
INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/it2/common.inc)
PEERDIR(
    alice/megamind/protos/scenarios
    alice/protos/data/droideka
)

TEST_SRCS(
    conftest.py
    test_bass_proxy.py
    test_card_detail.py
    test_content_details.py
    test_div_render.py
    test_gallery_selection.py
    test_inability.py
    test_search.py
)

DATA(
    sbr://2713351865=fixtures/ #  visible первая галлерея с видео из OTT searchgallery_first-ott.json
    sbr://2713361263=fixtures/ #  visible вторая галлерея с клипами searchgallery_second-externalvideo.json
    sbr://2713365441=fixtures/ #  visible вторая галлерея с карточками актеров searchgallery_second-person.json
    sbr://2713366611=fixtures/ #  visible третья галлерея с коллекциями searchgallery_third-collection.json
    sbr://2720103277=fixtures/ #  expanded_collection.json
    sbr://3111054655=fixtures/ #  private_practice_in_external_gallery.json
)

REQUIREMENTS(ram:32)

END()

LIBRARY()

OWNER(
    polushkin
    g:cv-dev
)

SRCS(
    GLOBAL image_what_is_this.cpp
    image_what_is_this_handler.cpp
    image_what_is_this_render.cpp
    image_what_is_this_int_handler.cpp
    image_what_is_this_continue_handler.cpp
    image_what_is_this_resources.cpp
    context.cpp
    utils.cpp
    answers/answer_library.cpp
    answers/answer.cpp
    answers/tag.cpp
    answers/similars.cpp
    answers/entity.cpp
    answers/ocr.cpp
    answers/common.cpp
    answers/clothes.cpp
    answers/barcode.cpp
    answers/dark.cpp
    answers/porn.cpp
    answers/gruesome.cpp
    answers/face.cpp
    answers/similar_people.cpp
    answers/museum.cpp
    answers/similar_artwork.cpp
    answers/translate.cpp
    answers/ocr_voice.cpp
    answers/market.cpp
    answers/office_lens.cpp
    answers/office_lens_disk.cpp
    answers/similarlike.cpp
    answers/info.cpp
    answers/smart_camera.cpp
    answers/poetry.cpp
    answers/homework.cpp
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/bass_adapter

    alice/hollywood/library/scenarios/image_what_is_this/nlg
    alice/hollywood/library/scenarios/image_what_is_this/proto

    alice/library/url_builder

    apphost/lib/proto_answers
    web/src_setup/lib/setup/images_cbir_postprocess/intents_classifier/intents

    kernel/urlnorm
)

GENERATE_ENUM_SERIALIZATION(context.h)

END()

RECURSE_FOR_TESTS(
    it
    it2
)

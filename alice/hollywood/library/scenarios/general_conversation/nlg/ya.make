LIBRARY()

OWNER(
    g:hollywood
    g:alice_boltalka
)

PEERDIR(
    alice/hollywood/library/common_nlg
    alice/hollywood/library/gif_card
)

PYTHON(
    alice/hollywood/library/scenarios/general_conversation/nlg/generate_microintents_nlg.py
    -i alice/library/handcrafted_data/ru/microintents.yaml
    -o microintents_ru.nlg
    IN alice/library/handcrafted_data/ru/microintents.yaml
    OUT_NOAUTO microintents_ru.nlg
)

PYTHON(
    alice/hollywood/library/scenarios/general_conversation/nlg/generate_microintents_nlg.py
    -i alice/library/handcrafted_data/ru/dummy_microintents.yaml
    -o dummy_microintents_ru.nlg
    -m dummy_microintents
    IN alice/library/handcrafted_data/ru/dummy_microintents.yaml
    OUT_NOAUTO dummy_microintents_ru.nlg
)


COPY_FILE(alice/hollywood/library/scenarios/suggesters/nlg/movie_akinator_ru.nlg movie_akinator_ru.nlg)
COPY_FILE(alice/hollywood/library/scenarios/suggesters/nlg/movie_akinator_div_cards_ru.nlg movie_akinator_div_cards_ru.nlg)

COMPILE_NLG(
    easter_egg_dialogs_ru.nlg
    general_conversation__common.nlg
    general_conversation_ru.nlg
    general_conversation_ar.nlg
    generative_tale.nlg
    generative_toast.nlg
    microintents_ru.nlg
    dummy_microintents_ru.nlg
    movie_akinator_div_cards_ru.nlg
    movie_akinator_ru.nlg
    movie_discuss_questions_ru.nlg
)

END()

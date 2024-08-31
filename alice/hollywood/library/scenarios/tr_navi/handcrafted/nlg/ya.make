LIBRARY()

OWNER(
    flimsywhimsy
    g:alice
)

PEERDIR(
    alice/hollywood/library/common_nlg
    alice/hollywood/library/gif_card
)

PYTHON(
    alice/hollywood/library/scenarios/tr_navi/handcrafted/nlg/generate_microintents_nlg.py -i
        alice/library/handcrafted_data/tr/microintents.yaml -o handcrafted_tr.nlg
    IN alice/library/handcrafted_data/tr/microintents.yaml
    OUT_NOAUTO handcrafted_tr.nlg
)

COMPILE_NLG(handcrafted_tr.nlg)

END()

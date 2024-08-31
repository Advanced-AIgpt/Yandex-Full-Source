import pytest

from alice.analytics.operations.dialog.sessions.intent_scenario_mapping import get_generic_scenario


def parametrize_name(arg):
    if isinstance(arg, str):
        return f"{arg[0]}_{arg[1]}"
    return str(arg)


@pytest.mark.parametrize(
    "intent,scenario,expected",
    [
        ("alice\tvinsless\tmusic\tgeneral", "alice.vinsless.music", "music"),
        ("alice.vinsless.music.general", "alice.vinsless.music", "music"),
        ("alice.vinsless.music.meditation", "alice.vinsless.music", "meditation"),
        ("alice.vinsless.music.morning_show", "alice.vinsless.music", "morning_show"),
        ("HollywoodMusic", "HollywoodMusic", "music"),  # for some time we used this (wrong) intent in HollywoodMusic
        ("personal_assistant.scenarios.music_play", "HollywoodMusic", "music"),
        ("personal_assistant.scenarios.music_play", None, "music"),
        ("mm.personal_assistant.scenarios.video_play", "personal_assistant.scenarios.video_general_scenario", "video"),
        ("mm.personal_assistant.scenarios.quasar.select_video_from_gallery", "personal_assistant.scenarios.video_general_scenario", "video_commands"),
        ("mm.personal_assistant.scenarios.video_play", "Video", "video"),
        ("mm.personal_assistant.scenarios.quasar.select_video_from_gallery", "Video", "video_commands"),
        ("personal_assistant\tscenarios\tmarket\tberu_order", None, "beru"),
        ("personal_assistant\tscenarios\tmarket", None, "market"),
        ("personal_assistant\tscenarios\thow_much", None, "how_much"),
        ("personal_assistant\tscenarios\thow_much\tellipsis", None, "how_much"),
        ("personal_assistant\tscenarios\tmarket\tcancel", None, "market"),
        ("personal_assistant\tscenarios\tmarket\tcheckout", None, "market"),
        ("personal_assistant\tscenarios\tmarket\tcheckout_address", None, "market"),
        ("personal_assistant\tscenarios\tmarket\tcheckout_email", None, "market"),
        ("personal_assistant\tscenarios\tmarket\tcheckout_everything", None, "market"),
        ("personal_assistant\tscenarios\tmarket\tcheckout_index", None, "market"),
        ("personal_assistant\tscenarios\tmarket\tcheckout_items_number", None, "market"),
        ("personal_assistant\tscenarios\tmarket\tcheckout_yes_or_no", None, "market"),
        ("personal_assistant\tscenarios\tmarket\tgarbage", None, "market"),
        ("personal_assistant\tscenarios\tmarket\tgo_to_shop", None, "market"),
        ("personal_assistant\tscenarios\tmarket\tmarket", None, "market"),
        ("personal_assistant\tscenarios\tmarket\tmarket\tellipsis", None, "market"),
        ("personal_assistant\tscenarios\tmarket\tnumber_filter", None, "market"),
        ("personal_assistant\tscenarios\tmarket\tproduct_details", None, "market"),
        ("personal_assistant\tscenarios\tmarket\tstart_choice_again", None, "market"),
        ("personal_assistant\tscenarios\tmarket_beru", None, "beru"),
        ("personal_assistant\tscenarios\tmarket_beru\tcancel", None, "beru"),
        ("personal_assistant\tscenarios\tmarket_beru\tcheckout", None, "beru"),
        ("personal_assistant\tscenarios\tmarket_beru\tcheckout_address", None, "beru"),
        ("personal_assistant\tscenarios\tmarket_beru\tcheckout_email", None, "beru"),
        ("personal_assistant\tscenarios\tmarket_beru\tcheckout_everything", None, "beru"),
        ("personal_assistant\tscenarios\tmarket_beru\tcheckout_items_number", None, "beru"),
        ("personal_assistant\tscenarios\tmarket_beru\tgarbage", None, "beru"),
        ("personal_assistant\tscenarios\tmarket_beru\tmarket", None, "beru"),
        ("personal_assistant\tscenarios\tmarket_beru\tmarket\tellipsis", None, "beru"),
        ("personal_assistant\tscenarios\tmarket_beru\tnumber_filter", None, "beru"),
        ("personal_assistant\tscenarios\tmarket_beru\tproduct_details", None, "beru"),
        ("personal_assistant\tscenarios\tmarket_beru\tstart_choice_again", None, "beru"),
        ("personal_assistant\tscenarios\tmarket_orders_status", None, "beru"),
        ("personal_assistant\tscenarios\tmarket_orders_status\tlogin", None, "beru"),
        ("personal_assistant\tscenarios\trecurring_purchase", None, "beru"),
        ("personal_assistant\tscenarios\trecurring_purchase\tcancel", None, "beru"),
        ("personal_assistant\tscenarios\trecurring_purchase\tcheckout_everything", None, "beru"),
        ("personal_assistant\tscenarios\trecurring_purchase\tellipsis", None, "beru"),
        ("personal_assistant\tscenarios\trecurring_purchase\tgarbage", None, "beru"),
        ("personal_assistant\tscenarios\tshopping_list_add", None, "shopping_list"),
        ("personal_assistant\tscenarios\tshopping_list_delete_all", None, "shopping_list"),
        ("personal_assistant\tscenarios\tshopping_list_delete_item", None, "shopping_list"),
        ("personal_assistant\tscenarios\tshopping_list_show", None, "shopping_list"),
        ("personal_assistant\tscenarios\tshopping_list_login", None, "shopping_list"),
    ],
    ids=parametrize_name,
)
def test_generic_scenario(intent, scenario, expected):
    assert expected == get_generic_scenario(intent, scenario)


@pytest.mark.parametrize(
    "music_answer_type,music_genre,filters_genre,expected",
    [
        ("filters", None, "naturesounds", "music_ambient_sound"),
        ("filters", None, "meditation", "meditation"),
        ("filters", None, "comedypodcasts", "music_podcast"),
        ("filters", None, "rock", "music"),
        ("filters", None, "musicpodcasts", "music_podcast"),
        ("filters", None, "familypodcasts", "music_podcast"),
        ("track", "meditation", None, "meditation"),
        ("track", "fairytales", None, "music_fairy_tale"),
        ("album", "naturesounds", None, "music_ambient_sound"),
        ("track", "historypodcasts", None, "music_podcast"),
        ("album", "comedypodcasts", None, "music_podcast"),
        ("album", "podcasts", None, "music_podcast"),
        ("album", "jazz", None, "music"),
        ("track", "pop", None, "music"),
        ("track", "nonfictionliterature", None, "music_audiobooks"),
        ("track", "actionandadventure", None, "music_audiobooks"),
        ("album", "selfdevelopment", None, "music_audiobooks"),
        ("album", "audiobooks", None, "music_audiobooks"),
    ],
)
def test_generic_scenario_music_genres(music_answer_type, music_genre, filters_genre, expected):
    assert expected == get_generic_scenario(
        "personal_assistant.scenarios.music_play",
        mm_scenario=None,
        product_scenario_name="music",
        music_answer_type=music_answer_type,
        music_genre=music_genre,
        filters_genre=filters_genre,
    )

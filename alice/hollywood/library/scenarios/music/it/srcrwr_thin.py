import pytest


@pytest.fixture(scope="function")
def srcrwr_params(bass_stubber, music_back_stubber, music_back_dl_info_stubber, music_mds_stubber, apphost):
    return {
        'MUSIC_SCENARIO_PROXY': f'localhost:{bass_stubber.port}',
        'MUSIC_SCENARIO_APPLY_PROXY': f'localhost:{bass_stubber.port}',
        'MUSIC_SCENARIO_CONTINUE_PROXY': f'localhost:{bass_stubber.port}',
        'MUSIC_SCENARIO_THIN_CONTENT_PROXY': f'localhost:{music_back_stubber.port}',
        'MUSIC_SCENARIO_THIN_RADIO_CONTENT_PROXY': f'localhost:{music_back_stubber.port}',
        'MUSIC_SCENARIO_THIN_PLAYLIST_SEARCH_PROXY': f'localhost:{music_back_stubber.port}',
        'MUSIC_SCENARIO_THIN_SPECIAL_PLAYLIST_PROXY': f'localhost:{music_back_stubber.port}',
        'MUSIC_SCENARIO_THIN_ALBUM_CONTENT_PROXY': f'localhost:{music_back_stubber.port}',
        'MUSIC_SCENARIO_THIN_ARTIST_CONTENT_PROXY': f'localhost:{music_back_stubber.port}',
        'MUSIC_SCENARIO_THIN_FEEDBACK_RADIO_STARTED_PROXY': f'localhost:{music_back_stubber.port}',
        'MUSIC_SCENARIO_THIN_DOWNLOAD_INFO_PROXY': f'localhost:{music_back_dl_info_stubber.port}',
        'MUSIC_SCENARIO_THIN_DOWNLOAD_INFO_MP3_GET_ALICE_PROXY': f'localhost:{music_back_dl_info_stubber.port}',
        'MUSIC_SCENARIO_THIN_DOWNLOAD_URL_PROXY': f'localhost:{music_mds_stubber.port}',
        'MUSIC_COMMIT_PROXY': f'localhost:{music_back_stubber.port}',
        'MUSIC_COMMIT_PLAYS_PROXY': f'localhost:{music_back_stubber.port}',
        'MUSIC_COMMIT_LIKE_PROXY': f'localhost:{music_back_stubber.port}',
        'MUSIC_COMMIT_RADIO_FEEDBACK_PROXY': f'localhost:{music_back_stubber.port}',
        'MUSIC_COMMIT_RADIO_FEEDBACK_SKIP_PROXY': f'localhost:{music_back_stubber.port}',
        'MUSIC_SCENARIO_DISLIKE_PROXY': f'localhost:{music_back_stubber.port}',
        'MUSIC_SCENARIO_THIN_SHOTS_PROXY': f'localhost:{music_back_stubber.port}',

        # Because 'stop/pause' command lives in Commands (aka fast_commands) scenario
        'Commands': f'localhost:{apphost.http_adapter_port}',
        'Search': 'nonexistentdomain.yandex.net',
        'GeneralConversation': 'nonexistentdomain.yandex.net',
        'Video': 'nonexistentdomain.yandex.net',
    }

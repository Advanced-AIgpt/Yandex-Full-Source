from alice.hollywood.library.python.testing.integration.conftest import create_hollywood_fixture
from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import make_generator_params, \
    make_runner_params
from alice.library.python.testing.megamind_request.input_dialog import image, text

TESTS_DATA_PATH = 'alice/hollywood/library/scenarios/image_what_is_this/it/data/'

SCENARIO_NAME = 'ImageWhatIsThis'
SCENARIO_HANDLE = 'image_what_is_this'

hollywood = create_hollywood_fixture([SCENARIO_HANDLE])

# For available presets see alice/acceptance/modules/request_generator/lib/app_presets.py
# TODO: Should we add more platforms?
DEFAULT_APP_PRESETS = ['search_app_prod']

DEFAULT_EXPERIMENTS = []

DEFAULT_DEVICE_STATE = {}  # NOTE: you may pass some device_state to the `create_run_request_generator_fun`

DEFAULT_ADDITIONAL_OPTIONS = {
    'bass_options': {
        'region_id': 213
    }
}

TESTS_DATA = {
    'clothes_with_intents_tag': {
        'input_dialog': [
            image('http://avatars.mds.yandex.net/get-alice/4454135/test_000/big')
        ],
    },
    'clothes_multiple': {
        'input_dialog': [
            image('https://avatars.mds.yandex.net/get-alice/3939230/test_KRmAUzEaHmKNxZE_0kFj6A/big')
        ],
    },
    'clothes_multiple_tag': {
        'input_dialog': [
            image('https://avatars.mds.yandex.net/get-alice/3939230/test_KRmAUzEaHmKNxZE_0kFj6A/big')
        ],
        'experiments': ['image_recognizer_common_tags']
    },
    'tag': {
        'input_dialog': [
            image('http://avatars.mds.yandex.net/get-images-similar-mturk/40560/XLb_4HknAh7eq/orig')
        ]
    },
    'dark': {
        'input_dialog': [
            image('http://avatars.mds.yandex.net/get-images-similar-mturk/40560/XcvbdjZJJjmZd/orig')
        ]
    },
    'porn': {
        'input_dialog': [
            image('http://avatars.mds.yandex.net/get-images-similar-mturk/38061/Xh5e_9je4aM-v/orig')
        ]
    },
    'gruesome': {
        'input_dialog': [
            image('http://avatars.mds.yandex.net/get-images-similar-mturk/16267/238c80ddfb39d59cb6e38ad881c1319c/orig')
        ]
    },
    'ocr_voice': {
        'input_dialog': [
            image('https://avatars.mds.yandex.net/get-alice/4364415/test_text/orig', capture_mode='voice_text')
        ]
    },
    'ocr_voice_foreign_text': {
        'input_dialog': [
            image('http://avatars.mds.yandex.net/get-alice/4055497/test_french/orig', capture_mode='voice_text')
        ]
    },
    'barcode_text': {
        'input_dialog': [
            image('https://avatars.mds.yandex.net/get-pdb/199965/5b112b6d-a046-45df-bbd5-37f1d5905601/s1200')
        ]
    },
    'barcode_uri': {
        'input_dialog': [
            image('https://avatars.mds.yandex.net/get-pdb/805160/3bee4166-3c79-41cb-bdc1-b10985ba1fae/orig')
        ]
    },
    'barcode_contacts': {
        'input_dialog': [
            image('http://avatars.mds.yandex.net/get-alice/4303059/test_e72bfb24de00fc097c69e5d0315ca2c3/big')
        ]
    },
    'entity': {
        'input_dialog': [
            image('https://avatars.mds.yandex.net/get-pdb/49816/0dcd90eb-0534-4c35-8525-d3572c2f2ebf/s1200')
        ]
    },
    'market_with_card': {
        'input_dialog': [
            image('https://avatars.mds.yandex.net/get-alice/4247871/test_img_id6526000481435545741/orig')
        ]
    },
    'market': {
        'input_dialog': [
            image('http://avatars.mds.yandex.net/get-images-similar-mturk/15876/XrDErXXI95Tg7/orig')
        ]
    },
    'ocr': {
        'input_dialog': [
            image('http://avatars.mds.yandex.net/get-images-similar-mturk/40560/588abc3b0369f792738f9074d115d832/orig')
        ]
    },
    'tag_with_ocr': {
        'input_dialog': [
            image('https://avatars.mds.yandex.net/get-alice/3939230/test_MIheuny8UOWLJgJ5Vo8vPA/orig')
        ]
    },
    'similar_with_ocr': {
        'input_dialog': [
            image('https://avatars.mds.yandex.net/get-alice/4474472/test_Foi2Olnz-NY_ndGBUDFziw/big')
        ]
    },
    'office_lens': {
        'input_dialog': [
            image('https://avatars.mds.yandex.net/get-alice/4474472/test_office_lens/fullocr', capture_mode='document')
        ]
    },
    'face': {
        'input_dialog': [
            image('http://avatars.mds.yandex.net/get-images-similar-mturk/15876/XZUKh5QPCXI-7/orig')
        ]
    },
    'museum': {
        'input_dialog': [
            image('http://avatars.mds.yandex.net/get-images-similar-mturk/15681/c445b6e90e3ca7b06dca29ff36ef3a5b/orig')
        ]
    },
    'similar_people': {
        'input_dialog': [
            text('На кого он похож'),
            image('http://avatars.mds.yandex.net/get-alice/4387275/test_computer_vision/big')
        ]
    },
    'similar_people_frontal': {
        'input_dialog': [
            text('На кого я похож'),
            image('http://avatars.mds.yandex.net/get-alice/4387275/test_computer_vision/big')
        ]
    },
    'similar_artwork': {
        'input_dialog': [
            text('На какую картину это похоже'),
            image('http://avatars.mds.yandex.net/get-images-similar-mturk/40560/XLb_4HknAh7eq/orig')
        ]
    },
    'inability': {
        'input_dialog': [
            text('Что здесь изображено')
        ],
        'app_presets': {
            'only': ['quasar']
        }
    },
    'inability_elari': {
        'input_dialog': [
            text('Что здесь изображено')
        ],
        'app_presets': {
            'only': ['elariwatch']
        }
    }
}

TEST_GEN_PARAMS = make_generator_params(TESTS_DATA, DEFAULT_APP_PRESETS)

TEST_RUN_PARAMS = make_runner_params(TESTS_DATA, DEFAULT_APP_PRESETS)

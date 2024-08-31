import logging

import pytest
from alice.hollywood.library.scenarios.music.it2.thin_client_helpers import get_first_track_id
from alice.hollywood.library.scenarios.music.proto.music_context_pb2 import TContentId
from alice.hollywood.library.python.testing.it2 import auth, surface
from alice.hollywood.library.python.testing.it2.input import voice
from alice.hollywood.library.python.testing.it2.stubber import create_localhost_bass_stubber_fixture
from alice.megamind.protos.scenarios.analytics_info_pb2 import TAnalyticsInfo
from conftest import get_scenario_state


logger = logging.getLogger(__name__)

bass_stubber = create_localhost_bass_stubber_fixture()


@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['music']


EXPERIMENTS_FIXLIST = [
    'hw_music_thin_client',
    'hw_music_thin_client_playlist',
]


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.scenario(name='HollywoodMusic', handle='music')
@pytest.mark.experiments(*EXPERIMENTS_FIXLIST)
class _TestsFixlistBase:
    pass


@pytest.mark.parametrize('surface', [surface.station])
class TestsFixlist(_TestsFixlistBase):

    def test_smoke(self, alice):
        '''
        Тестовый запрос с помощью гранета фикслиста преобразуется
        в "плейлист сказки Мифы Древней Греции".
        Он должен отработать так же, как если бы мы сразу задали этот запрос.
        '''
        r = alice(voice('включи фикслист крест ухо зима стул муж'))
        assert r.scenario_stages() == {'run', 'continue'}

        # NLG is hardcoded
        assert r.continue_response.ResponseBody.Layout.OutputSpeech == 'Включаю Мифы Древней Греции.'

        # music is thin
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField('AudioPlayDirective')


@pytest.mark.experiments('bg_granet_source_text=H4sIAAAAAAAAA-19a3Nbx7HgX0ExzpZIOUqch5OotlYly1eJqyzb11'
                         'KiSskuFkQekrgGcXgBULJykSqRjCx5JVvru_fDPu5-2P2-tSBFiBBIgn8B-Ec70z2P7p4eAJIgr1'
                         'N7mdjG6Xn1vHp6evrxTwvrzWqjaC-vlc3Nanv5TtFs1crGwsWFdxbeXqhXG-vmZ3Pb_N6s1ix4rf'
                         'ZlvdZqX1hvNtoGulVtfVGr11sLF9eq9Vbx9kKrUfx5o9qoBkC7-LJtkv9pYaXc3CwbP62u_kOx0q'
                         '7dKVpYx8VbC7XNrbLZvlgJOQwWt1u-CfP_t65AyoXrGzafAVTM34_rxeZm1X8UjerterHcutcoG_'
                         'c2W5Vqve6TVmstnmZ-tIq2Sw51b8Yio8FoOL5v_tkd9Uan4yfmuz96kSZ6MEVxszp3FE0rp4DKsf'
                         'lfbzQY7473Rt3RsUHsdPw4IGaQ6o6OTNIJB1MAwfSj4u688TwdDUcHpGnzuY9AQPxUQ-PjraJRrM'
                         '4bkyMzFGfjvfHXrFmHZHe8Y4ZzOH5qksm8woyO75u8B-NHNsFke5okmsHnQz7eyZaiybzcLszjHk'
                         'Pgx2YPrhTLjWK9avdHGIQvqyu-X-d4g6aTLzopyNZOwGb8X_DP3vjR-MlisnDr9XlPw4ldptoeOo'
                         'HVLEBmmZt_umakvjVoiknrmaU0tGvejPKQbEbYig9gfpTVdbOofjHnTplRPDbd2k8bu7bdqq1c2a'
                         'g2zYQVTUMlaytq27asXYIwY6EnB2ZaduywcKDt8yPoPRuQZ2ac7psxG9oxGz8UiabmPVjkDPzcjO'
                         '4BHTy7SyzQVgb0RVAUyGCmytSfyeAok0HFIs-SBib3d5wcDNwq7Cdgg-4uxex4_K3pcX90QvPZRX'
                         'MoKAlZTUOTmAwFJnUN8IEdSi1Dnw-5XWpDmKKHyRoEsmF30JmtivfizK5B05OnpoNds4IZbTmzIx'
                         'T6L6h231R7kmBm8pkq7WkzeiESbDu2TxK8Z0qYubD7nDQ-AWlT4hAWRV-d3vFfYeR2HcW0GPZk5Q'
                         '-hDjn7QAoHsEYZ-BsYxftmxdHukj30XtHSj0_A-xlMsp2hngYeKkA6RcdmmT20u0kC-nSWu7AiaA'
                         'LB78Z20xxXHzcCjow8m5G0y_5bO1Cj0w7_HA0loCsA48cyB6CfAC0JT4Hd8VOteG8xYmc3mtlXxw'
                         '47-gnYMUBXAAA7lsNjx4EOOwFE7JLiBLuu5WzcarwPTEPXYqmCLbZ6QjeTYLHXS7heZBKxN7lE6F'
                         'W-WtI7xpF0BIMylICuAFjsFZ4mASK2EghYpsV7i_ra_rC2vtHOLHA8Mp7HTrgPj7H_HLJPj75PJc'
                         'gHUMQygkJvaMEM2mtrk_BG6rwHR6nBnH1a3DlgKAAWf57D9UAAsQ8SCL1Ii_PFQbFjn25kOXYM4E'
                         'Y3xU4Awwin2KXFCXb2GLIsQhy7-OnGjgCGAuDGjuSIY0eBYewY0I-dKJ5dAzm6bMpwyswBSP0eJ9'
                         'SZg5ACPlYptAR7KijBjg6mlZABx7slwVYALLYS1E1AFluZy2GbgBHbFAzYapXICfigdb0Ie--c4z'
                         '679PLX0YAGbQ1sUFdzI_p6EnQhk2S7ka2QHZCPXaaDeEgKkFsXHOhWhsgZ14ZMCKsjSfDrQ6lKjv'
                         'nvDb3L8kqGIRtqAor3auuZS4m4XtG7YbtZNvRiwM1TvpEU-8PWVtHMIAh86gO48IlSH5a6XALyHm'
                         'ldMiVy7dgyz7VWrtTLFhE7pIecu7R36Ic_1fxnl336Q45c90Vpf5qF8v7Ei0XEofAA7jVnZnT3HC'
                         '4c5M8CDuwqQH9ICKA_JkSt_kwQ9frzQ1YiV-Z7TXYUswXz1PBI7AaxA6thl93CdFkIr75oqmAQEm'
                         'roxDWCa93eIpYMrTIIWcKGR7T7WFRzBkwNwB3-HMQ-e2kOpV5EwF7zaOlhXAYnsP-hulPIbsnCoB'
                         'PlWLD9TPnFRUjsWXGfrc3_DLhSAPnoydTeKzduaaw9KqD58NGjnFIEsc9emuNV0RidwDX7IZ6T8c'
                         'OjIUH0s5fmeGU08JpziGjEj4CGANHPXppDbrH3q01dsmXG8DshAo79YTJIP9Y0Z2hy5s1oEUm2og'
                         'UqGxHz6tvw9ZerqG9u64_VO8cFxeudzwpJiAslCGR78k1Cluqr7v1AxCaR05etlBDE6bS1M5U-iy'
                         '10s9rcnLQaz9jaIQDy0ZOpmdl9ZmVwZjgextkVIPrZS3Nk6n1uZWxhvONndi2c4fTHqs0HlIJBf0'
                         'VSd-ak8FbS6gieBIVu6wkJsJfL_cqLCQXATlLYYZ9hliVQAHpaLkmZr5T11cyy-h5GhS_j-XZSLi'
                         'hl3vkovw4Xc-bYiNfeiC-7mNnWmN--fVk0wqbsTNzk9D64vVnTX1LhGmmlaLMf69fKZgEVCjjAlK'
                         'M95NcXfkAgjqMA0c9emkNbgCTD8FVPGHMmPoDTBaVf7tTAD8lyNe9l7sBYyawMU7OW8kvNexq7ZH'
                         'NmCEnEskPRFSPkUXvFNUjGtzOnKWT39u0soXwGkvSTuOgJQFsJLtli_so0p48rwf54FOlJX1kJf7'
                         '9dK7L3yVx5OTWuwVelUrHDnamjtUDVPr7MXMx3kAk0C0Y-XgIflSaSWj9pFndq5XZLpz74-nloaM'
                         '-h0HlwJ8ijcAlZ-PztqClE9IBSNaGtZtkotxuantCfCh2R19Bk6FoBtP84DD_He2ao98QIl_PXq-'
                         'mRn-Nd9gHN008zmoNRn0jA4Bx6BiuNoTlv_Z8gC97cLFZr1XZRvydSPsE5u7FRa8FVClawFZ_uVj'
                         'pOf8n-sGoQAzO0p3akDQB1Q1AEZdOtOsKOhR9C4SeL5-Ma7oWH36e8vwSrN9Rv0rvzae9YpwLGUR'
                         'ckvl6fmC6gkNrJiskEcyWImHRmGQ7b5CVs6YUbJPtudimeU3azPSTl4D44hO3Z48N1vV7enftI5b'
                         'D3YKKGwHqNyhA9s7K7bLFzxQZWJCZajYzxE0LM3DieAp3cIwmeJKO-ktzXH9ba7Xox7xEZ2GdZ1-'
                         'lBxIZrTg0ZePxk_BUbCCKF54vlCA6Ph9lx9aOgAZ_TdnOj5s6FZ7YbHvTQiZAHAmDwiB1E4PjJ-e'
                         'RgXZm3wpa2xQ5gyZ-kG6wb0qLu3ylkszcce3YnhPSPRYYvfA2Uh-4J7wkd5z5QdkNUIjCZ9ecw1g'
                         'O6wa1OpiUJJwvyUae20rZs-7yR71kdA97Yx43CtnSjtjn31vpLlbRL5ytwCJhTY8mdK0uIEMHpcq'
                         'Nsb2Qejl4dnx-v1er1otmqlGtrgV8AtZdn5IYQD-Hz6vGRIPsG5kmMg4CSGWO8K7wQ9hL8rpTNZr'
                         'Eyd81nOJyc7tyuOO_gdZRuAI7Rzeyb5esPGZ021_NwoFtqkUXbJmqou01vFc72yREtFux6tdZ4Qz'
                         '1SpvsovJVHDOE03aVkCQ8Ye1h1KRC0oDxgF44TQRIu1831b97r5eBSBfbZsb2yRGK_Awp9Dzwg5C'
                         'DEVLlBBpA9IY9Rc9NPCyAjLviumUegS_PE6YEzilubu4r1OackCkoJhsovRvKLpGc0hHNi_AA5nP'
                         'tiErbMNerL2uabYIthofdBf_g-XebPLNuOT_P7Vnk08mao7Xoss9r1k1LDK2XjTtGoFY25Ex3gaO'
                         'wBHABSbxR6ZnfCiOmi4mU2jj6oxqINRMJf_76Y_8FLLgFWfQDUcsdP8OFn1_0H-bBRvxMyPJW6VB'
                         'vzRw1kGYIBuby6WmvXyoapZO5Lz-yB8Xcp-QmoDAgA5ugMFt6pzikPsI6T8V4lpWM3yvXiDfARhn'
                         '6ewPTsRkQtReaazsjehZyncqMQlYic0Ce8sHmQe8kL_MwMVikT3_zm_3AXXpdzmkzhNdODyGtnXA'
                         'buOhMA8Xn0Jfo-9VV2-ts_eSGY8nI8XZ0hjvqkF_PJyiGkjikv5NO1PLQniglTlwr0Z5qDuYumvT'
                         'x-orz_ZTDMyeuJvNP-p9pYnWAXyS0n355iOClSmbB0iixVJP_jdtGylDqXLu01yUB-WLTnLYuNhP'
                         'dmtRHIJZFhHAJ73Y1C5kP6YQUuA28G-JCKU0_wnLDGcR50ykh-Mq-2XicNzvCEPPsJFzkdWkMhbc'
                         'jmzxiA8iuRgrk92vWC_AV6ldvcKnQ-FZT_vGJqn1ZIE47ZHQGUnc04mmE8RM44afFa9Yvig_b18s'
                         'ZGVe96nHKb9bw7ww2XY9gyezLvd_wPqzkMP0Hsc2nxUtLY5fVmEdibtz5oNOLRnbQ7ZdhnGHqKvb'
                         'vgX8MOhOecJRi980oBJs-AdavlYvvArcr4WOHIqhUbPg2bwK8-8cl2jZ9WkDmeWiEiuQmSRGEdKL'
                         'McOjGsKCsMB3niAG9bWgKIIdlBjgnehJACg1YCBVpxqdNo74K0LMqGYzroHhAST1Kc6bcZDJGWK2'
                         'S7f6KARl1twKLY1H_KKbGgngIaf8sefjABGKh-Ol5noM6_o4DsCx8HO3UTuKz1eFcwEVSZmeGzTw'
                         'QpJrDNZjOaSb9EJCgsKzePFLjCNXLgNgxPBP7XX857o1OW5O0hKdDuu7NIru2fJNmu3hN4b2AXfr'
                         'CePXLnxCk9cOzoEPFyH65W8mCJsgZ_0iRHD2H2ldMoadhbPofHADshPQO08pg-Bb5gGMGwCdAD59'
                         'bgUageiWNCQN8vVuq1xpuSRn5YTRW7rbjtnBMW7Nl3P_ckjsbsIEnoWTCV6vcNgJF88v1ReXdxSb'
                         'QRiDNcnW0D39oqrcnBQbxdhXd8sBKWoz8EEekLeOUmT042zz4R_gbyc4QW-7DtnuR5CPv45WSG7m'
                         'ilPgYQhx4XObHj3Mzi_CWzZ3AT9WZBjBeApPFXQBJpgs4oA-vAM_MOWNuuaHD7vZzYmkkn6YijBV'
                         'rnMcmZuSYJ9sTcx3_sJtWSD-Hx1i63A71dsOO0wmNO6glYLeHMn8gQx-EiLKr9KxtJP6dTTHsyx1'
                         'yBdMAHX4kTV4O0RQ2r4UeVuhVl_KS1VazU1morlbbJYVCtwOUjWS4fZg1QnD2pYSKmLEzVLHYaPm'
                         'trOkLSOpVh9Ay9Igh-OgN2Kh7JjlNyR5vT6dtQt6bVN2M0s_x-diM1l1Q22-PcdrN7aYadTO0bkz'
                         'rQqQOtfuLWCY9-FHFlX8w2FxvFp9tmbHJTlMzO9XpRbL2x6zbU7qC34Ki1zkNOziM7cXr-82n94p'
                         'dhNFI8w7cjel6SBOqCB6WCAFZA9PmJ5tHT-46tEsPXLre-15XdMg3yBTt0Ll-Si8EhCJB24ao1TB'
                         'l6pBd4ATlNTwbnBkiy6HiDs5LvgVYKkuymEAlOTyYtARvJCtHFpcryw32Qoz8Ew89d-0yhZLBs11'
                         '-tQ5pM2gPBzuOFEN4fX2TAhKWGJN8nhQpQ8-D-hCTZO8nfBSBqfz1NElQRCiSe-SsxtjclUfbNP2'
                         'IMVWCSm1y6dnmHz5xXmnSWHROhgvkIONCZAmJ4eOD4Gwrc05t5gPTbIjeFEgv12gB6BG6muryt6G'
                         'qnL9jrJIPcQ8DH3ec9AhDvT7hdTeekEjoDUxcYOGVF4O6HY-qIaGK5qYTXX3t3jzQvP2o4Gf1wLW'
                         'RAVv4WPjgvVc6RE8LQsw4zse6wo6mDVHURxVlLFZAK9pY-J1zVZwuWKH62gGxUpb1RbVdWqo3K7a'
                         'KyVtbr5d1itXL7XqWKSiuQC8pdLZsV05vNrXrxtqmErhQ7CI_tc4Q9pzyP9NmCzSYprldy2UX68J'
                         'k8HT6-_Q_0LsXOM514TyDcCvXKUK4s0cgSjInEIiEUKpHIEAgJzpX4mnQr2fjJphfbSGx0bQuFi3'
                         't1uzVvYcQt1JWwSzSs7utFG3Txwfxm1yodUz4NsAAOyJbfsl9-UNytzlT3nJ06LKFPgSdOZnqU3g'
                         'lCEgcbFgc2f18FaoL6oetiP5uQ3Duc3HIo2uEJfHEiAl22Psw__BB1Ej51jHhSn4OVwcAE_SzXE5'
                         'PWguaw6KqayJSh3OjFMyuCUNvVg-VQ-lWpiEZwrAj_ytcT4Z35tgDqO-dtsWLrpGg4tpIuhuAaIg'
                         'zC5DsOOuJI0P-kXr13eXu1NndbigO8MgDiTrhn-UtCCLkaV5hJAeToXi-qzRXdGcvr37yu1hrSCu'
                         '_TYqVcb9T-HGbD-Sels4ESun1YKUwWfGjuV19zEDyGyFE4wcuvX6Gkuxaj-Ru54GNqn3QpRbTvAG'
                         'L4t-bv5xO0BA8YjURNYpfQBWJywE45Mo5iN-ZeNufguNj23eMcuBfi0QGEF6dkXA3oa_Fg45yRnr'
                         'rx58jfKObvGfYURpJT59HQ7UYppSFJXb6-yRzJ_K6bgp93Ox_ZLPtCdAyckSYbwtcoN6lHLEmOPR'
                         '-vTwsDbn-_IjIYIXeu9LMJ6XUzJIY3e4a2Z9Sp4mWqSX99_hoL1AMWm1pUqNT0D8I7DbF9i89EGt'
                         'fMpX5C1gRPx9nyQxU7s1iszPFYHh1DeAjvSmYhKb6nsiY57kB_gUAHr5Jh79tHKfvorB5f9rT9Xf'
                         'UN2GaACnWylVOw6fdjBsz12RVNl2A753c9Pn-wt3QJlnqW8r4tX1EyueWajAlUqwh15mneU6_Cl-'
                         'Y7pS_HHsQ4zC6I1Pb4dQuAsJYoh_lyjy7sUfZ7lvlfAs1RYXUMRp_RmmbRHcmiIKpqmB_jb4C47S'
                         'rZztympArcJOF5CswlIJYSCArDceBvcSvCJU9fP0_G-nLrpTkZEDt4S2q2rLiaTTjhuGf7nXDsnu'
                         'iUcIZasHX-_hQOUDXRPWqDQO4gQ5c-LVpb5dxZzVueFp9HvhkUjcPdPYL4bnavRKCK4q5zYxEgYA'
                         'j3CWWEd-G06YLba5XRmJC4k03mY3Wl3I6mEE7qhJafTBOSgYVMSCtxli3hpzcpkybQUjtqOxLMSi'
                         'i5k5wLgmWtvrHrmBVzbq9603bgVPbII4m9VXKApdkMALpUFIByM2Q6SC99Ht6396orX1BPt3MzOX'
                         'UsCV_yB1yXCEDjp1TSGGWTAvgfRz14NqXiiR-97F_KqPiO_6hisK-UaxWvOF1pl5WW5QMqWyZbrb'
                         'FeaZWbhUlutIugIPmjSnujaBWxUGuj3K6vWvlys6gXd6qNdsUckpXbZXujUrUSiIrJVrlTWy1KUR'
                         'VhwLr0jKbgFyqQ7juSwLi4yNwpICoPYmygVm8_Uy9lNXSxiNOtmpJpej39Ccmcy0ozYFgHxt_iez'
                         'wdXP9EnwDYQCl6AgRIepFokD-35JfydUrXI1jmTBlJpVupXk9Gp4eBHyns5g-Gz3XMOg0XktwHXp'
                         'YXJpTIhpEA_tZJErr8uCbJ5PVLTRSnnfrOQhMeqQlcjhyuObxpnvRCBSv3RAXAj8jHSmME_CIBRS'
                         'bCE9e_-7IWGQi4RbKtK6-V9uCzzjIOCXMBMm0lwpN-M6NBlabfrVlAI0vIkc3HOy1SOcN0oROHzx'
                         'Zidqz-s4WkHtZu0G6ttezpIc98UCrznZiu3EL7XkjhLblmWfA51mXndjwB-SAFHDhUgD5YgeqLXE'
                         'kYhaAFqpNyvSpvMEXm-cPaWvsPuvaRl6cww5BZ7aZ8wQ798HEH_OeQffqYA-HTW4WF0j7WQCjvrc'
                         'JiEa2L1tP4--Xd3HpW6SOcZ5zIztT1IY0XQj7A1T8PHkI-wet_GieEgbD7HIQBALQIIYCN6wRiQz'
                         '-sB_8dhg39tOY4Owk2HATYCNAQrUL1eCVkQq6Vd4rLWznRj5PXH4KzBf4EN2uwNV68k4Js_1PgUA'
                         'HasVCALjibrNWOiVKvi-yWVKKMzZVqYyXacUWFJKKvHkB9BRStExiQXw-8sj1RO5khe-SVme6lCk'
                         'yqVtlaliAtzT8tNs068SMB7B7Kf08kqE8A8iTeA7bwmOZxACUX0fqKoPTIxTlC_D4115L4asAyCC'
                         'Dmj-QFRpQ_nAl-ONViDUd4wjlr6rRcBVcFcoq3jyIfBcTrPUVXEOBcicNANygxaDeDtB3l1LfOxd'
                         '5beuEOmE6czw6dSXD4CYcK6CXgKYE_9xYXgwAIeTH09iR0DghY6g3EpIStiwnsljgEvtVHyOxnE_'
                         'j1jSfShaYmkhaZKjTlemVCroyUPgWeyZyI9bKqy-nw4iG1nAQzD8oEMhcuLw4MteHTK7ueunuRlS'
                         'hSkFOME7r1TltOy0kJDrIESW6cV7USMMlnEvpAbp5R4HD8YHyfyufDU0a5WbQ3ahnnRWgHaiXiPx'
                         'kpLlRi6jhaQ6JF7E9AeLlPoHaiUyjmTev2-bUUKMNahLzjxHPL77aLlm4QDjL9rnwmC0BuPZXmpG'
                         'B5AzH0sti8nYsRk1k2ch45MN0EV8vmejnhCWo_XDcYZk6PIzckYIrGtQw8UEa0KTe3cio_Zrk9g5'
                         'MXaCLf_GfWXYytEB9chrC3PCGlb4fOWA3BoJTkPg7QsSvLEUNHgpYUIR3R2wU8r8oGT0GYoDZ66i'
                         'ZqhwK0xp0e-VQE8PFG9ti9pai9HqZI4LOQA2q5Jyaq6OOzvJ6gI6wOpc09cRQoCcdrb_6ejhauXU'
                         'rtDgA1Id9LmARFvUiVO2VPJ3-G9jDcrXuRF5k4LqRjv6-tZvcEV5hCuaN1207JT-QLaaVFPXPRdY'
                         'Qx2gRHP52OoelWHBUZjr_mz-KH8Li2z4cOvY-J9j8tMoete2pLXnHgZQq8WCWCqCMgCzvibAOSHk'
                         '6rpL6jUVdrxFtiM0Uymo_04cZGMxfpSzKPOus4gLUuOFQG8jaqWeAeY-JGGDBNnCtKzoAOlS4rRX'
                         '0-obR0mO9gNP_XN8kJvEsS5uIRMuyC35DV27FROHJQqjJL_pG9ZSkoRDUZOoJRyWX0osIlgD5uxC'
                         'HBhgDoabVRZBy9uoWJmJGNGA-90BRXUXS48YLpa9wzYItPUx5WxybV53FzmIDBX2Jin41guKJk3A'
                         'J-WljT0RulvPohHFi6S3haoupvx1OWMZqkQ3iFjvUUb2iO4vAE65kQh08ubwakXTlwdCRo_vN3H-'
                         'cZQwHpGsplo11rbOvUOXPb4jX8sbqyvb15pV5UM93DShTrj5hBSxJ4Zuund0upDNGfKSlp7GrORW'
                         'RgP4-lXMXrpiQJp7kS7pFNSZpJMEkxCXGACSBELyagYQIKIY1JLhrUmIJHMYIxA8dox6ISEb2WYS'
                         'sAXpIssBUgL1FWsE3AUbqsYKtV0mM8Mcf2NB1bCRomIIutzOWwTcCIbQoeYiCKiWPLFxLGS5UgjH'
                         'IugUMFiOHPZc4QAj1JGLmI52mCi4-uVaUISq9v317JWkt49zPkuSgpXc8HIHUmJ_rDKVqU7CSJSQ'
                         'Nl9gnNm7TsTsawWa1N7p98oOE1fNBYaRbVbA14XZEKXnvAkKP7obTbmhJvRrd3Mm7vF9NwwxfipE'
                         'p8_H0y9SVR1hFfEveche8T4WHBqSeIbqSLYEFcKqrZ97NTlE6hmxc-LsMkAZ0iydzonCbJi3c6Jb'
                         '8EzHQ0EDw79MOTLf85ZJ-eXIVPT6hCaU-iQnlPnGIR9lYV-tOhH_hWNSRYsKF6LD7xpWrIsODl8Z'
                         'VqqGIhp6cjAfEJk2KUzOpjBRSfMzl2aX3xWTODJVsWHf6JL437HD--jh4nAHxt3JeYiXrwtXE_g5'
                         'VclR0JiK9ubDblYn6sgOJ7m5jZpL741pYZuzBqZLzISJExYqPDxoWNiGhF8stlfcLbcwjDJ97FrL'
                         'hkT4Bne392NT5HRbCOBGDIeQ4aJiCMQs9zhSj0AuxWSgJ2wenTSug6dh197nEln4gpBQwFALGkOQ'
                         'KODOgx5ECHnyyusRp3a-2VjRt_vLJRbTRyTjZV6xSZ6FVxxGFY25x0yj9zRoyZi45Baj2eo68eAi'
                         'O5tyivwfY4VcBT-u4jKokgBLwi3ivrMfRGUf24eaVcWyt0NoEfwQJ4o3yvrMkn2Bul2Y5fEMQSnS'
                         'Xi2RdDguT9-k5y2zshzNmHxVr7KlT9hjSd_1S0PKR1r3Whdbeoep9ewZ6BODUVoD-V2y7yLYoCwK'
                         'FeNKcbHYcX11tO1dN65ugn9VwrlnDqwdzZCn1xrJdCcfTxAwI5c6LoiJDoJUPncgKmLsjvvOR27H'
                         '3LRhGutToxy9HKglEMFdwA2o4FNQnwhxF5Wfdsc8lFZOA2KCMSmilsifADcfCfAydIoREJRty1KT'
                         'gqxmc4H-1rxB2FOgpwwL1AOptmKgM_Npz8I6bjyLBR_ES6djWfyuGtHVgvOObOIkNiSXk_hEwNH9'
                         '6ZzGLFqVfshFkSR1SfijHwISp8LmIItvF34MAZRgxeizsA_TaYrIyf2KlfjPN2bFl_Xw1ghvYGaA'
                         'v4JMhyElfP8uLhhGgPvLmaX5Y3q_cM-TC78XK9thJdEX9EDPCS-qZs6Bk2NeyUc-55xR5Uzv-sPe'
                         'fsAqdBUaygzwLhhgM5Fi85jhbeLWAkB7YUGvj0Ycog1F3Y1Li7cP4UEPdS44EDHewismUSIaAF8-'
                         'jzTdKsFfkC4zAAMB02zZlhRNHPDphrXceRpbP0lgIL6EFzDDW3xnpAOSTeLhEdTFh_fk-zibhvk2'
                         'QDvp8DZ9qzpPiFnoALYldDJCRa150ycQDt9UmUujiw1tkps1AGMHpDfgy-XfQEDcMTfIFJsXOkzw'
                         'YDAg2C4zQDSH3TkvgOhwaGsj1Y6PcT5NE7B7wppajA4jWERBmmkIS9G8rkXfBfrFQZKXDXvyX4tL'
                         'cMs1KsxyhR_u_H1cbKRtlcbpfLt226SL6VshRLYWVrB_LnSbN_F830lEaL4MEiaZKc8qHFtHrolX'
                         'XhCy3FZK2ac0BKbWa6aReDhZ9W5haUOc-LXAj4eIxe03yJc2vx1crc-MhBnrAQ9Ah3p2M8mocQJS'
                         'mwMZK8ChKcoZpZSqtQ5mTziH2BlG8C2_EAjmNLPUJX0TzeqSf4fN9EtuQB-6Bj0sWq3OdGUa-XpF'
                         'LguQ5gyA6ouoCSxMwpwvu9OftOwBsdIHcSp8Z5_LZ8nUsVaXCuujvwEzXNDspD4tHQ8B2wtx-yoK'
                         'SS7Y1Jx5GbxAio4XMfPk_jhA3I8jLrzrFsCg8c1iHEgmGlyCJL2E8AhMI-AoMSYyMFOf4Ih4edX-'
                         'q5RdxBVsCt3QtSNYcorCq0z0q56g4slwgSWsrAPbIdg-2p5eiBlDfoawswmDYTvPfh3Zmo6u3LQB'
                         'kACEvQIEmCqu7AIzbThH4AF-VTKeVlTD9xB4R-PuHlevyVB5IW9mhILBbu2nKD6E0lxGKT8LCCx9'
                         'R9v18Hx5TS8UNNDDNd99Zz5rEAjFk0RYKwFhlinDjc0-IhjIM_WAXfUQjEesmT30f2pmWZ3X0aIb'
                         'ZLo7KSy2hEhVzoSP094B76IYzv8Ui4-D_DUCb4XrzjAjwQ2QIRDly8RWO4ffQK_jysmgLv1RFTs_'
                         'U1SK8c_pn-1EnBUimMQei6wXTuXqNArYkqzYCWE0dZoFG027XG-twjxIvYEGEcCNnDQ4Qy27BU7q'
                         'OEzCb75RKcK4fdyTzDkG37VwgpPjD8ZX-EKoXHSpISF8AMxbXtVm1l7iG-UbWed7KHyvmRFvUcLc'
                         'pgdrNsztufFwzz_sgHbEia_Li5Ov8gfmgbz05AEAzYk2DENK2G3tnJyKlojkJAnfyW-mg7q9b8Gg'
                         'Nl18tfQQ5L1hkAyeEMoqsdmMeBBB7T83aIEUA19K01_-1q1AMLCgVj1Qy9MymZqAKoyd-Shzmn6e'
                         'Zk4ZHCdXJJ8LSWSZIVoxEvau2yamVCqDRJ-DZ9OPAuRucfxkO66HR-XgCfAIygSaT1Slkv9dVolo'
                         '-_AiSl8oERnDYfsUNDwNd-Rej0fSXnMgbW6wCo4qFOBK7VGtvtjCrBSTTO04r-vtzO9B1Eh1qRbD'
                         'j0pDspHDFN4RaNcCSNYyT7pPE_FdWMzgrIYrUdnU5dcSfziIWcO-xAcv1Bt3tfoaQQeJrs2XRlo9'
                         'pcn_tyt1ZGA9Cq30PuezQMhy5EZQG_WZaD5qx7SGB6nUjHE4ArmnbJKhJu6xoeoDIy5CT3oZOrMq'
                         '6fA5M2MOBrI2tk44K74pESw792U6d-dn0Wm1tFs9rezsQDHmPQUCQZGJhE881mV6WN-1lrZ-wLSN'
                         'TP_Fl3uZZZrsh4HboA3yk9sGznH8v6dmavnaORv7H5jmcrF8-rwzLx-RZPbpDwgsoQcGUgQ19C8y'
                         'zGo_nMNCFt0C7pu9XMLLgnk70cTbu-VRQTTPOGXqikD_vNaitvi3Yi2dkTVEGD2MtJVRDLJGOoDe'
                         'v_mKgl2Xuixwg3PXn5QXYiXoLMKm2VyjXoE4Rfjjq18a27Vl9tFg0JrldbLRsNvSUTynq9WN9O4F'
                         'ebtaKxKqG_a1Ybq1tmwhptmfSxdVt_tbpZq9-7BkZpMsMnajEDNbs6yXy9drtuLzMSvFUaXqHFZ8'
                         'B3ed7e1MDA4cQt8BP37oFSxP7oRfSrdgAmMAP2yWUaaE95CDzQQ8oeaw8mIMjYpcIOBPTZJy5xFu'
                         'vb3-h5XUnlQ2osI98nfPhGMLgaRCAKxlAqE4Qsp_4F3D15BvWSuIGt9Oyhs877it8Od8AvbxwMOJ'
                         '72UYslAiEX-3AdHfBF4FbXvM9VOCFdBEp-bNJLH8QSZZ94DBPQeJebb5yMfMCrAStKL9cn8JDeo4'
                         'JEDIn3FfkMfgHCiJ-xcbaCPYoLumQ75APvnDbh1YwPLN3yb2B07T_8cyAAHH0H4uNml94h-5Cb7J'
                         'DX4kC8ljN3kxZIELBegrVOQCQ3GU9HwuY9lE4fjH_2gXPAtWTloHHj9oL7LoGep8dvzKm3J-CR_q'
                         'Wz6R-KiZQX1VTQ2uhpAqREJ8qD7Vs_0Ipjx7ETsQRyw8x8Dh3LnpKHCw50D9NksNwhOe-5dO-0_P'
                         'OIaGior7lO6_4p2cYaGJZFJMXw0Oacvo-_FkvVHcJz3_bIVYtPeuAEEFsZ9prrpI0KyC74ZLelXM'
                         'ncJ8vtJULbD-E9hFyohm4FkkV-xJ9IBmMf0-aULnACpkuc7l8mjjzGazEob_bpcmcJdPp3UCgnPs'
                         'law6sU8tMChDdhqq94CtqsZG31mLh4jPFcn9LPr2P2R6he4eNtJizqvKcOQi_D0JBd5Pz2-a1CXh'
                         'h5Ah3E-_CmNYzeD9kOzCWnU-G92RyylmWCK0NHKHD3c5fz4nOo42DPRmd8TNRkyuj5DDTyuVKeJf'
                         'v-kWtQs9gqW7V2TbsLXb5dbr-0m37ydDbap5SSRMVOXoNOlXvoB_O-dkTh5IEim7r8pqJtfNDgvX'
                         'R0Y8Jw8CBhR8rYXM0ITSfNyyF9K0ZWSgnT0ix1dfDXcukftAyGhB6SPZigcbPWnrcvZjytZ2r9xk'
                         'uHEIpLizilok81E9b-UJFxv6eLvV4v-Lbv8kOnMH-UNPvxS7uIlrud1_eHxis8kCELptTVrmWsHw'
                         '6VOXyvWCtz0q9D-laOY3EoyWKpv5I7_S9NPuSSfoESJD3xmhS_e_i9VlFf09M-KVuGr2-ZFTOx6h'
                         'sbVUl5QkotkQ0FNXc-agHN119-q7XNWmO5Ud0MC93ZYcRPoi50Qm8kJ0S3jahJ4JM4_fCyFekfIX'
                         'bvZftBNEqExsYYw6tT9zHxOCGo7IRsKUJ2JsKy-T5iYsDzFQ-xStyfBYDMwcOVfkN0yAKgLwHDBD'
                         'A14CqoK54yvpyBhzxGRgS_UMDWg5kGfqxW4nh-ip1q4Kbi3QJnvw5wCy06ZHgOZXd-vzMv5oxH8p'
                         'XzJWZrtj4h0Zpfr1AqnAB4PxDUT0FOy5Vdx4diimeZ3CibJuOhaR5PGpqEZr_0IWhvTsP4EQhl3z'
                         '9Z6cfOy5_fTNltxIK-Ub-v_uqojSN_aPHEgByn_2iumPoV4-9dys35R7tDx4bszg9CXojSGsm6U-'
                         'AzXPYlPrAetd9Hn1NYJa-_PzqpjKKrM6LxGxS7mDqjV6NVWrq5UWRYlkkzaHW5o1IjfdRT6p_7nW'
                         'ogtckDIONsMqRyJ5JgySeqQlkc0YVQpuba9tyjtrgHoGN47_H4ncPpRkeBLANajTm7r457zhouZj'
                         'H-sIxvo04N8hlVLX25tlkNZMu1Vkz_m7Uy3XJXq7XmvRvVzLsqKPcc2jdodO6IppcD9IOAH-7dCy'
                         'zdxyHa4WJSAS2dT5RWBSEDq110bru5Vl0pWnn7WBtz2xCovImsFHtw8exmtdl-v7hDbe34ZzJuU5'
                         'bcDMvO_q1CI5xf6aFFB7dN24u6oWiPEBZLMGIuV7fr8roBHYMQlEVTS7pxR554f6qaO9yXSzzbeT'
                         '8cnyvj5qoPA4c1GHSi9oM6eugejoWRh8004MaAJ2jzO_1k94XJI54DewcsSbw2njRfqpKOqBuoJT'
                         'ZEqcmROgOx-HlWPFopH8JL1Qvwyv2NpRcQCA-B9rGvX-HgfpqzL_L9w-14Rw_i874rPTQZzLYyVG'
                         '695u1pwKoSHkBMYlE39IgmYBMd-KiEr_p6HPq3xFpS1825EErqK9SQHLhXAXi0oJ-O1Cy-VmEw9Y'
                         'm2Ty-UHXDjTlj88aeK--tTjPYdvoiPgaieSOb_gH9SdaP4-sASBxrwDDQ_e1qJvixhTWQ4o-58tV'
                         'LQUwjSOPgedpvYPTfuSNYdCWaYOf6pzh5wePj-_eR76AFiFG0rrZA0duxq2YwfN8olSdiXRHex2n'
                         'lr9sCGOQHtCNBXXmJm1Djk8RwnXiyoJUp5s9qYdFt-_VChwJWOevxgcEDixiCCgwmXvhCcuwDyIA'
                         'dgjG4_dI9zT_jS6cFVICnDIxs4UA9uGM4tql12LNm7kEmBp4wOsAgJHugRDOrRMiGq-eGy9FGe98'
                         'eP5Zp6v2y06dyBiRKfVl7gRvm7l5a1T1d96Vkb7nivOOTGkSLdg54THUsAvhAFLVcQtTD3sB4zf9'
                         '8KUCjkALRia0ZIqgVXD-aSSAG-FTZK5hzMOflEP_Q9dEAXx525HE-qqxeFrlTcsim-FvTQd5a6JD'
                         '2Ce4jwAXvkNM_OUif7Q72aM5pAOQs9f5IXQtRFLHx_5AOLt4F-LOpkQ8Kcfr_WI5OLFpV6cQU_4X'
                         'vcn3Iu_3gnKcFnsIgGC04P5RE1EvQ2GQI45ACcGwY6AN8CEaD4hELvBYoLpjTBT74CRnWu1J2jHs'
                         'ltRzjVZoPB4tjOT_uERJcVyOsJZ5kS_iarJe3kygiLZ5knt9YHTo1ioFgrODy-Ivo8tCl4z-CLbC'
                         'OrQI7OrNXQz_kkcD9xP3HQhWBNl9xgkHP47mMEHCnUY6R4_h_lwyCbzV-9k-ln35lZhgYOvMKudJ'
                         'SJ4FwLnzTLbKgNfkBeioqkaNnLhusE_AeIHFnTpvL9wkaszRgdoFoweTdxDuRpiO1vlUohsmFGMY'
                         'TPCp1_lxBjU8h1c4xO12VSiBmZc0GX8-R9gvrYwKhcSuZzsvUtOoka7zje4FQOgfUQl9sYwgPHEa'
                         'q4WtUb6jMr-MOnlGDXKX33ZXsQPjpnNSVc3nn39ZIEea_2ZB_wCuSoy5ppJVqD0pN4UjPr0odFNe'
                         'Ni3nvuV5pOEgDRp067SeN0shbkiulnWGSc1zlIhnOkRKVN7DLTEx29EmVZkJePeqpRQBfWhAeBUO'
                         'MYay4ICRlQvE-nAaJZB7IxRrTTUlumNBZJHMRk5ZRlsP-lHFHGq5rjJ33cB-H2WIL56sk4fwTJCT'
                         '8DwHe1BI7O1KZ5b2qtdmbegb8li0Z2ybni4x1Kgd5UnCUwHLK289QCnjYjXQhMO2Cvt8tw33iLGX'
                         'UlTQIg4c3ESzE1_tLwfh0lwUyEwjRB0svJ0RKVO1SMpSJ3H0uQpdAk9mFaiiUkpZxSuxIZXCTJU1'
                         'tpbZhvy9MUJRKhksRLzhKpUykVhniHlcmNfLitJAw8-pENGvKTkxhtRz-VkoUnQC03jcATQfG6B8'
                         'tVcPi4e-jI6HECvI7fSMYolFvzDzk_-MFTprlOKNHSlaSUo9NKWpcNFq0-ik_BMbF7wgphyyTdWY'
                         'wamzRIg9o8690n2QBlKCuX44aRQY5HMkirnttXknJVPo6MUgqvYTyJrg6aTCpOsvA7oSBGOr4xQc'
                         'N4hB4kk5gMsio2wO9tZwbYxeeU7MkAKlSCO6AKheBb_HWuSwRcvHlwWvTSPoLQUZF-FznSgH3nMH'
                         'riYvtdPXdbfQZsPWMNLMt5nHJAmVvQ0PsKEPTUObdIg025JH2egxG_kLFMA0oPfkp44hvlBDcMwe'
                         'HBc50Pec_5cMhw7KK0NgWfFitFLXeZj7fMZKvCmzKxEuVo1ctc5LF9uIIPEgYobIFcBk6DG7ezTT'
                         'i9opnq-bTWyFFzH0Ko60RpAlWfPEzlujFhkK6zfKKjf3pycMKsoxOSOTqsr1ebGcLzPBUyHYGvCE'
                         '32pIKd2TVN4Oshe_tHAtuTg4GnSgLGE7bHQiyGT95imdN9P0jxP3BjegbyP4W3TEqg5EGCnckaB8'
                         '4UB4JiAKbznRQEQZ4S4FABjh9Hf0mxAyGMWPj0QcQiYCgAtKbYZxf9YZfXxAFDASA1hWGy9dAP1J'
                         'middBPrGGB35KKvLRzvMviMMAEcRCnwuSSnJ5FuylzrkUtHGMECC3_eCefkFY106oJ9XVI1V36MS'
                         'QfdC4losc4pyoYYuioCcNMAp9r2UoKwnlPa0-BvGYvEoQ6wwfWFj-H7FNZRdb9Sv78O-FCJEuwBI'
                         'h_cls0MNOYHv7KVQFhr-xz_1rty3qt1X61cBY0FEaSaP9Tbaxm9fyol5eZtACnxNIQyUKxOUkXWp'
                         'hpOlNkTJLVYdm0Lhh_ijnyaXRQzP_N_tus4GhU68vVVstMR7XRvhARhKLLNqLzspsuv4hqrQCptJ'
                         'vbfveuNQuz0CikVS-jJwsAbBUrNdteo3W3aC7XGmslTbZ_7XtbhenvdqtdbiIOF_xqsdlF7la5be'
                         'jIxcpb4IfyKmZ0eZpljKVu_25hpgvWZeASL-GV5NaYc237F61BFSD64tYSSg36waa5mdSq7aJ-T0'
                         'n9sAgeeyj4o_KuAiVmZ0oqUS5WUokyq5LKI0vQDDh6aZ8RDv9OwXawb9ZWC0eU3i9Wr5XN8s85jh'
                         '68eeBTjeW5jhjculAOKdHainkAyZWNYGZHlDQ4LZX6BpAVo9l5vrCWPhpObNvaCkxMZ7WnnopSDN'
                         'Q8DIt8pnxVfG6zr7TWjXbUbdlxEjT61hwrmRCWSLqjEmBrs-xBuAov129vC9Dvy3pttXrv06JuNu'
                         'QqT_uouGtdG6ppdkETIpMucwotV1eqIeut-CQwOnaO3AZLcPFfQpdMYJtvD9HzzixkyV0-logHrB'
                         '66TLXnNI1I3UPlbzpjA3Ckc986o2Ofjp9mDq0w8NgZNf6JcTOA57dlOgqQml85b0xPoseRAPAomu'
                         'u6ln_8VxuIY_wEOIqDNPkpbm9a9lRuqCH1kGvl5RCLAK56VOUleu53nBgZCveOHp7BZAZ4vOmxIU'
                         'lAFEnoFuqcQ176yfLBdDvn-XYf3Kw12rmAsEcgmWB4RxAZIAR2-WfUUM6afCSGIbHzwX6D4k4tN6'
                         'h9pXMzCQ_Y4QkbO2Zrbb36Dreart_jdmZCaTLqVgFpN6g6sbMhmJZZbYW4YoXvk1Oickps0Mf3xQ'
                         'eMZFB68OOOgqADZxrjJXvfjne5WK7LnDahDoPlx4lZ7xi9cDrXQJFCAxg-YcHMRJQJ9QXlVLB-Mr'
                         'ufbUHnKW18ny8YEJfAKyf1lTD2bq1PiWmy84zznIBQTR3eIDmRIE2AFyMrwfiWZn8hUjmyh4kfe5'
                         'g0NyRRtb9YnW7yCkN6yADjbwSA6Hn5HAwQF7H7-nIhWkgYLK5vrxmW-SKv00rPhEJ-NwVBLm5Z5P'
                         'IxYLta_4Iyq1-S30jhIkY_xitVfLDy43SeILu0kH3gOof97-C42IBuEuTm4po5aovm5cbqtWpz3d'
                         'xh2tUJq9XSL4gCY06dDnw8ojZnf8Wbtn16Q7uI8UOfCF7y7MMIOhqKC8Vp13KXYDY-DPMQnHFMzg'
                         '81YqN64E4yslkfJurZqW2cOh65kehi7LXxfRvXBb7vw8vPfXxOmjFjoBNXyka71tiOphOemDu4io'
                         'V_2cEoEEyD7UeV0f-EiD8nNsQHeJK2aFU-W8B75GcLIK92KnA9CJLS8xKIf7FS8hBmexcUr_po8_'
                         'R_oiHChdG_xpgDIHw5uVDZaLe3Whd_-tPVcuXCPbCO-Em7qG5eaG7_FO_V1a3aT9a3zTnx00UYcH'
                         'KjDENt77MG0-yFNoQCD8Phv-9U69tF5Z_I1vpswbrP-GzhovnV3ig3q63ldrNaa3xm8tBcLXParW'
                         'wst4sv2zYzrUJWw7K-nWS02LuM7Watsa7lATwx0w1AquKc9VWc7qkNm3lScWk3LMKVP1Wbq_bnyh'
                         'etMAD27y-8I436OlY8-ufw4G9OuIu-sn_nm3p7trYuhMb-4v5LhQIkysSS_-1UYz2brjGsFGqJX2'
                         'DaXejK-45PPerwT5t38TxSXdha500jS87Sf4k5I6TB0qasixXHMS3fLssvWstm2S6vWT5uuW1ZLr'
                         'lSytvWMHbZT_OklcKyTlopUzKS5VK117JJ8--qqq3OiJzNOANqejaC2Dvv_vqXv_rlu796hbVpaF'
                         'XgkvtAJgdAy57Zny4yaHA7rKzHc86F4Au0KIxB351v9_Fehy71RW6VbW5omrk1BVM2vr9oMTzHjw'
                         '8byDrgTAqqQNK9RYh3yzvYIS6UDdVdnHUdkyW7vFY2zWK-1_qB0Dg5P2w8wxQz5852x7_-UsrVrC'
                         'wiDaVzomS3g55q4dNGL3c-n7uvNElV6xpwecuM30phXZ38bUwW0Onw4G8tIew2eP2p0uudNlHn_I'
                         'uTdeMgyr7qxlmvNet_I5MRaYdz1G44s7ntGlbrzDvGuXq3U9EhVQBBFRP0Grum2qhtVuUs_b89mb'
                         'ecWOQHeTi_87N3f_Xz3_z6Z7-4-M477_5sbrv1CO9amXM52akrbq9aASD4fCHlOyCXAJ9ZEJz3wc'
                         'yrY_Nee6O1XK6ZRbFiONv28nqzKFaKv40d_K8jG-Drsbl0wTAcgHzLXLT-M0jIQC7zSrM1Q73KjM'
                         'GtHood8mLPYrHPs9PrCfE5X80lUc8lVpHn0g6AF-qOH3R8uUW1jZ_45JDqGkI1J8tP2RuCV-7ynu'
                         'tirWzoZ11d5nLQKFoFvRH8UFfW6L-h8MHKXZF5ZD1-pXU0uUqVGR_IIh0PGj99tTlI6f9G0Sx_uB'
                         'Ohckz7Tjd419oz2Oxzo8JJzbNzTYYS2-tQWkWHg17rNoJT1tosWhvVZu2L2t_StCVPVXO5l-Trfq'
                         'mps8RWq-TVJ2m1WV0vG39bGwtfJjEU8RxnR9b7cpuKFzeH26vPyd1ae2VDErt_43VzuFFe92fv_H'
                         'ZuywHeHqzw8qUWQgd9S1Z8rIlLi6Sm2UWUq7XWSnmnaN6T0sjNspw6wJhn0tjmcpBhJRjQAZUjwR'
                         'TGUF_DWsfbM4a4srTPUENz8CTPwSbreRLMeLxHfLnFmr2uh1apfw_ywcaGo955k69vZckHnaCXbB'
                         'uPMZP7l2afCvcSYWic_ZeVkS-XjeVV8BKQvCqkyntTKahWZCIlna1AmErZOCa70rFa6FlaEebGTf'
                         '3Zwru__u1v3_3Nr36Ty9euteuelv8Puwv6zlwcFEXMftuxGq56WZB0t2zhW0q6_dN6Mo-2036-88'
                         '7Pf_7L3_z8Xbb26d9fFPjnmX4122b5vEbHVspN6wej6ZZjtd5SyLXej3dNH377i19Nzd5avmM1Wr'
                         'dbL9FGXMIuRlY3P1q5mt5cx81J8Itf__xNd_xfR_979F9H343-S8X8y_18uTUjYDTPjIcXmsmPBq'
                         'CIEf2RQLDSi_C4a-fm7Up2U6TnW9DBCjdNlPaNXQglS3Cd02qn1e4fpc0NvOI9RHirSHMAEicz9p'
                         'AATaGJiM9InVeL1U2rCtkoi-XWRnn3h8PH2r4OUBkgo17rND11fiWer1Lbc6kitEjPU8D51xSiLf'
                         '-cD2Fk_CaPX8z3_5UAjb6Mp5smxEtzTuuCumHF6ZIex0iIs8zZXdALRMnUD1swRdQv2fS9TbyZw4'
                         '6vuDdTq5EAbge_wwm3g_9s2tZI9CSXKueCTU9Rr3eEqmJHKDQumq2DdVg9K1PNzLtn-4tmtVGrNp'
                         'bXyvoXfBbq1cb6dnV96i0t5ps0_pNykRmw-NQsQpOZdRv5DWi_ZcO9fA-uq0icQX5qxhCHCfRBOt'
                         'R2YWZp0BZqdZsFtvrDWaOMJP8vc1z_S2X0361pM_iP7E1bbTciBT6XW1jY7UVU25t1sKrbq7XSqq'
                         'Isb4I6GiikbHqFtB_K-FGdCbDLBZnvANxcT9ePmza4OcXE_FhftqP2nhm1jt_y1pVSJ2jW2UlIa5'
                         '11Tr4oxLb-N_lLDreXkL-02uYytLzWLDfdRXo6smqRyat5pgKkA7_9xS9__ctf_2ziK2mz2CqqU7'
                         'eczzUJv3wegtLlen3yjmEqefvjp6CTmfxYQFNSzcIxtSidYhbK7CoXvCrnhb_bqrXKVd1IIiiSB7'
                         'Xcb6wOMFhGhRDVXJ89Vgz2SC8RKIlqfFHwIVx5qA63c86CjvipV89b9Kyr1pN4RQ4xuM9Hz1zVux'
                         'Sv22YIaVv_PPovo__E0TxFg4PgjCsmQXRnYKEyYIIsJO2bET1lqvEk-GlXB_sHU1FVcF2EbgGZin'
                         'Y-w4hHWIWM6M68C5rDLCHK31zcWmtzcYlGvh2gv3s-WRhd-KsUSGLP6ovETs6_h4XyH2K2GWfZHy'
                         '98CQJ4yjr0B6UKJMrq3oUoq-IWaWcpaA4yuaiS4TkTnab6iNEpt0x67mxZYs9_VzSiB1u5vMEz9y'
                         'mLZYCOszPWYvbP7TS-nIBZeMiV2Oc-ifDvbFfIckzAoyF3DY95n-p5j_lce6CeUwRWQTAVeCT95V'
                         '3NGChoW4sVt0dFEn6zXr2ngGBtJYVvEm_fwXl9rbWV1nG1FmhgiJK6lRioXbcHdXqciVzo4Jd8ou'
                         '_HBLv6BJos76ciydBQLcmLpzJJppSW5NuqTGhMS4vCsHxzatqOM6sZoiDNKeTncoRODXl4E5kHej'
                         'cpzwM4AybkIMYBQ25CxXKdgFcOjNHwIpvLR8gAkwBmjqa0q1oPZfOjiqIhkbv-4TePa3w_mpxHbH'
                         '6tlqfTaxk_Rj1KSWtBy12fa5KemWmRQ5lnmkObZRUHMcc0T26GaZ7J88tanDq7FP9Jc8twVGc2yS'
                         'GJelKDnFWljWROzQCi3QLEos_uYZonv4tlLn0fs1yZnazjlO5llm_Cbmb5pu5n3vosO5r1acqe5j'
                         'jndnWaK93XSk3KztbaS_c2s2Gxtzh1h8tcuX2u5dN2e5JP3fN53OTOT3Jm93-ScwoVSHGYTguS3k'
                         '2kCCnuOl3Q80nqkKktoRG5VqdQiglVTGcmnaQy7_MqiEpVoBu4WB_zGY_-38V9HqTJWQ4Nvf1Rb8'
                         'QATlYED3pNlhZPYIaJacKpyD3R3hhGni0bPiWQoHByCgNHQQ-cm4I53nuYqMc7urJBwi43yvZGzs'
                         'q7N_56_B35oH4AwKeAVdIBf6nB4BrNLp_RMBgORHN5RyGPoYqvZRIG0ekGf6TUd9KUMKse4BSB8P'
                         'kZogn7lBiM55Z9S7qEbtisu26Qjj2GoPWXwoV56LwmPpGxxND_24g7Z0DkeTgsXEMWoej5QHO7kd'
                         'lxY3RUcci2NgeDL4zoSkjz25GvHF1DoFXtM3zrsW8enWwi85KilpxYwpkUkTwVlxGFkn95e-Fu7c'
                         '_V5urCRdC5-Mv_Bbahj-0jQwEA')
class TestsObjectFixlist(_TestsFixlistBase):

    @pytest.mark.parametrize('surface', [surface.station])
    def test_station(self, alice):
        r = alice(voice('включи бяка бяка бяка'))

        assert r.scenario_stages() == {'run', 'continue'}

        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField('AudioPlayDirective')

        # track within playlist
        state = get_scenario_state(r.continue_response)
        assert state.Queue.PlaybackContext.ContentId.Type == TContentId.Playlist
        assert state.Queue.PlaybackContext.ContentId.Id == '1106528703:1019'
        assert get_first_track_id(r.continue_response.ResponseBody.AnalyticsInfo) == '93474700'

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_searchapp(self, alice):
        r = alice(voice('включи бяка бяка бяка'))

        assert r.scenario_stages() == {'run', 'continue'}

        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField('OpenUriDirective')

        # track on its own (should be fixed in HOLLYWOOD-659)
        for event in r.continue_response.ResponseBody.AnalyticsInfo.Events:
            if not event.HasField('MusicEvent'):
                continue
            assert event.MusicEvent.AnswerType == TAnalyticsInfo.TEvent.TMusicEvent.EAnswerType.Track
            assert event.MusicEvent.Id == '93474700'

import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import voice


@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['fast_command']


@pytest.mark.scenario(name='Commands', handle='fast_command')
@pytest.mark.experiments('bg_granet_source_text=H4sIAAAAAAAAA71Y_0_bVhD_V6yUSnwb07T-MPFL1aFOmlRQtTFNFUKRIQ61l'
                         'tjINu1YVykhoskElH8lhARCHMy_YP9Hu7tnv3fPdthQ2VKi5t353bt397lv_lDZ80zHCqp112uaQfWd5fm2'
                         '61RWK99UlisN09mDn94B_G6aNpJrbtVxg2rN9oMDb2dlz3MCYO6b_m92o-FXVutmw7eWK4H1ewCrD5Vdt9l'
                         '0na_NGkje8cXzq1sVu7nvesGqkbL3PddxDxw_kwd_c2vEWnlj-auwNODztGE1m2a2sBxzp2FV_UPHdQ6bvm'
                         'E2GhkLtNN58MO3gpQd9-PLuJ8thvJn0okvk45--ob72IffxiP2MznSFnQ8Xya9eBKP8zq9f2SlMsmvhRs23'
                         '9r-ojEfT-MIvqP4Njky_jSSFhjuGn-0gTYBa92i8YAAD43hoU5yRMukG_eTNtKHtPl0YYndiiQO4xBZ8C_K'
                         'jI9CL-HAIew458QbIU-3wSs7CBrWY_tmCjcMhc5g9kgjJ6fJJ4UacRFQH3XOtO2SCaJ4kiPABSbSs4KYnOJ'
                         '9tpez-KhD9FjerPDQomf5AcHzyqoHP5Do_wgxEJ0pZSsHonVrKY-rN-4Bwgq9PACrtJLPAk0TBBbaeGE7Ew'
                         'aAuoaHAFbxuCBn3Vo04juQckWO6QBE2oi9RbkdhN4haEDARRyVK4IccmsP_h_JvaSajMoBQpClCuD245vnB'
                         'kTnX_EojYmMfw3a9sHZY7V__NxAzSQkkMeXISyGLAwQPYj5bHkHJ4zhUJUoQKGQ8kJfPkWkEyCNkx4Bi4jH'
                         'aGL49kg4A8Wv5uGmuwauftGwd2UMzW2YTbkoIOUf0PIvECPcir4PQam2wACacp5cCbkGLArejieYOcB36D3'
                         'yKyUgCY00g4fILCHJENWIk3JylEZ6ORPDVmMlZ4Vjk3PKgpgP0wxFtvzRcSzvZ2EHbtO5Epo8lIRoB6aoGx'
                         'Em89qkTHIyFjSZNYtMgZcCG8itWeQZ52GY3pQzhGOPyhSRTEihozwzTD6jq_NkkXQB1TMZZWpgJQrLVEjjC'
                         'iL3Eut8HBYfAA8A3go7EZ1DSFYd1jVkH8Joq6A8BiQF9qREFcJdCMgr2kKyxO0ihSj8zG2631t7tpMHz1PT'
                         '2X3retXAre4gP8feKhaDRQnEsty6XTj2pVO751DLqc06kiVseWJRPN0Kol6cpNhlYuYpT-HDPMYWlrbv2bN'
                         'Fe5b0LStSn0yjJw_96GlVr7MSEwD6E9k1qP5O1hKe6SPKd7KfOKLAVg1qIcfl8uCM1DUz3ZWkx0IY5BAuEt'
                         'U91ekYoiykYJdXvcYrUCIaqF77TFWvY23BbdIXopggKrgDMtMAC_89LKwc8hKUDOMLCOOTeGpQB4YKTZU7i'
                         'IudgOTmeLDzRra0pTw0RBe-rezYc1GFu8oWJV2LYoXY9WTLC1resr6AoQjghY2kwooSc6HgNtWGDdzFsFTo'
                         'PIggNwvpBnU0bboVm1TyJPSatIhWVUqrCdxrCBdAQeh95sYCJdfFyPO1Xam4gQGmjQCU3NqwMUShEIVlT4z'
                         'Azv1sdpGgz8hA4lUGfQIFf6jcT4S0m1cEiTpQUu3GpW6GCK7RojloxGXkWsBI_ZyKOoogTD5lxAkfYK-Uw7'
                         'UpF3stRCo1rQw2jC5BCyF_y4cawkHIE1qHuuvzcjNzqMMoJyttRgB5DHVM4QnyKYRutRyoEajFwKzV1YfHO'
                         '9H8luhLAihosyTbg1CgvvJC3QF8LbOBPj0oTUTfzqY2ffKCua1QfL587KrZTdupOlDAFApGbEbWo3yqwYVV'
                         'HnY9EcB80ZO-10uaGpceeg8GhJylMT1zJQdKyQFXpS0fKyqELygerNEZS7a0GPNFpC0os5WdawYPt4Q6lVW'
                         'k2YesH_pWoy7HMupZvmQuw35B-l4R9MlGkMZFUtqSUNiOcFxmYVfa5pG-WUuVe-C16_uW79vvrG9fW57vOg'
                         '81pnhHpBYS9-PkOA3NsheUEJlPjLWXG5svfvnpq2fPvls16N3CEIeBFl0TzIGIgOETo0nkYMxWXaKnF8cqL'
                         '-6VewPD394QH9-mgtowXq_o-lTVnT3XDbhfQaUJDUNdLO5YF65AoWmq0AV2ZZRCUbExqxd1re8kzIDdfAv-'
                         'OE3rUhnjfm3r9dnqgjn-T4XJvx-XK-_tP0yvlr5p_vg3f4XlFsEWAAA')
class TestDoNotDisturb:

    @pytest.mark.parametrize('surface', [surface.smart_display])
    def test_do_not_disturb_on(self, alice):
        r = alice(voice('включи режим не беспокоить'))
        assert r.scenario_stages() == {'run'}

        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField('DoNotDisturbOnDirective')

        return str(r)

    @pytest.mark.parametrize('surface', [surface.smart_display])
    def test_do_not_disturb_off(self, alice):
        r = alice(voice('выключи режим не беспокоить'))
        assert r.scenario_stages() == {'run'}

        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField('DoNotDisturbOffDirective')

        return str(r)

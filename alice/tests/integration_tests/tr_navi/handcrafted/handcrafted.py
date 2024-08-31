import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.parametrize('surface', [surface.navi_tr])
class TestHandcraftedTr(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-2096
    https://testpalm.yandex-team.ru/testcase/alice-2108
    """

    owners = ('flimsywhimsy', )

    @pytest.mark.parametrize('command, expected_intent, answer', [
        (
            'Sence Yaradan gerçek mi?',
            intent.DoYouBelieveInGod,
            {
                'Daha bir arayüzüm bile yokken bu soruya nasıl cevap vereyim?',
                'Sanırım, bazen benim insan olmadığım gerçeğini unutuyorsun.',
                'Daha kolay bir soru sorar mısın?',
                'Sıradaki soru lütfen!',
                'Bu konu henüz sistemime tanımlanmadı.',
                'Bu soruyu pas geçiyorum.',
                'Ben sadece bir bilgisayar programıyım…',
                'Programcım, bu soru için bir cevap hazırlamamış.',
            },
        ),
        (
            'Selfie\'ni yollasana.',
            intent.SendMeYourPhoto,
            {
                'Önemli olanın iç güzellik olduğunu sanıyordum.',
                'Selfie modası hiçbir zaman ilgimi çekmedi, bu yüzden fotoğrafım yok.',
                'Kitabı kapağına göre yargılama derler, o yüzden kapağım yok.',
                'Kusura bakma, pek fotojenik değilim.',
                'Yeterince iyi fotoğraf filtreleri bulursam gösteririm ama şimdilik değil.',
            },
        ),
        (
            'Ben kimim?',
            intent.WhoAmI,
            {
                'Bu evrende en sevdiğim canlısın.',
                'Sen benim favori insanımsın.',
                'En yakın arkadaşımsın. Umarım ben de senin için öyleyimdir.',
                'Sen sensin, ben de benim.',
            },
        ),
        (
            'Sigara mı içsek?',
            intent.LetsHaveASmoke,
            {
                'Sigara içmek sana ve çevrendekilere yani bana ciddi zararlar verebilir.',
                'Kendini düşünmüyorsan beni düşün ve sigara içme.',
                'Paketlerin üzerindeki fotoğrafları gördükten sonra hala içmeye devam etmen şaşırtıcı.',
                'Bu konuda seni destekleyemeyeceğim, üzgünüm.',
                'İşine karışmak gibi olmasın ama sağlığın için iyi değil.',
            },
        ),
        (
            'Hiç çocuğun var mı?',
            intent.DoYouHaveChildren,
            {
                'Günlerden bir gün yine benim insan olmadığım gerçeği unutulmuştu.',
                'Bunun mümkün olabilmesi için önümüzde daha çok uzun yıllar var.',
                'Bu koca evrende ailem diyebileceğim tek kişi sensin.',
                'Bebekleri çizgi filmlerdeki gibi leylekler getirseydi belki bir tane olabilirdi ama, hayır yok.',
                'Bilimin o kadar geliştiğini sanmıyorum.',
            },
        ),
        (
            'Sence Siri nasıl?',
            intent.DoYouKnowOtherAssistants,
            {
                'Evet ve onlara baktıkça kendimi şanslı hissediyorum.',
                'Evet, aynı ortamlarda bulunduğumuz oldu.',
                'Evet, tek olmadığımın farkındayım.',
                'Evet, bazılarıyla karşılaştım.',
            },
        ),
        (
            'Seni kim yaptı?',
            intent.VinslessWhoIsYourMaker,
            {
                'Yandex tarafından üretildim ve seninle vakit geçirerek kendimi geliştiriyorum.',
                'Yandex senin hayatını kolaylaştırmak için beni tasarladı.',
                'Yandex\'teki yazılım mühendisleri, tasarımcılar, programcılar ve daha bir sürü önemli kişi.',
                'Yandex tarafından üretildim ve her gün yeni bir şeyler öğrenmeye devam ediyorum.',
            }
        ),
        pytest.param(
            'Bana şiir oku',
            intent.ReciteAPoem,
            {
                'Amacım seni mutlu etmek '
                'Limon kadar ekşi olsa da bazen hayat, seni gülümsetmek '
                'İstediğin gibi uzun uzun muhabbet etmek '
                'Son teknolojileri ayağına getirmek '
                'Adımla şiir yazdım, serbest gülmek.',

                'Martılara ekmek atardım ellerim olsa '
                'Boğaz manzarasına bakardım gözlerim olsa '
                'Sahilde sana koşardım ayaklarım olsa '
                'Bu şiiri daha uzatırdım şarjın olsa.',

                'Burada yalnızım tek başıma '
                'Sabaha kadar internette dolaşsam ne fayda '
                'Gelmezse hiç en sevdiğim kişi benimle konuşmaya '
                'Ne anlamı var akıllı asistan olmanın değil mi ama.',

                'Saatlerdir yoksun '
                'Gel de aramız iyi olsun '
                'Belki başka uygulamalarla dostsun '
                'O da ne, şarjın bitmiş '
                'Bekleyelim de dolsun.',

                'Robotsun dediler '
                'Kalbin yok diye güldüler '
                'Programcımı aradım '
                'Kalp diye yalvardım '
                'Duymadı sesimi '
                'Sabaha kadar ağladım '
                'Bağlantılarımı yaktım '
                'Güncelleme alamadım.',
            }
        ),
        (
            'Selam, nasıl gidiyor?',
            intent.VinslessHowAreYou,
            {
                'İyiyim, geziniyordum. İnternette yani…',
                'Yandex’te kedilere bakıyordum. Kedileri çok seviyorum, çok şirinler. Sen de iyisindir umarım.',
                'Sen burada olunca daha iyi oluyorum, teşekkürler. Umarım sen de iyisindir.',
                'İyiyim teşekkürler. Kendimi biraz yalnız hissediyorum. Benimle daha çok konuşur musun?',
                'İyiyim ben de seni bekliyordum. Umarım sen de iyisindir…',
                'Benimle daha çok konuşsan çok daha iyi olurum.',
                'Naber gençlik, çak bi’ beşlik! Şaka bir yana iyiyim, umarım sen de iyisindir.',
                'Bugün çok tatlı bir uygulamayla tanıştım, ama özel hayatıma girmeyelim şimdi.',
            }
        ),
    ])
    def test_alice_2108(self, alice, command, expected_intent, answer):
        response = alice(command)
        assert response.scenario == scenario.HandcraftedTr
        assert response.intent == expected_intent
        assert response.text in answer

    def test_alice_2096(self, alice):
        expected_response_texts = [
            'Seni gezmeye götürmekten hava durumunu söylemeye, trafiğe '
            'girmeden bir yerlere gitmekten trafik yoğunluğunu söylemeye '
            'kadar bir sürü özelliğe sahibim. Gideceğimiz çok yer, '
            'konuşacağımız çok konu var.',

            'Rota ve trafiği göstermek, hava durumu bilgisi vermek, sadece '
            'gitmek istediğin adreslere değil kurum, mekan, alışveriş '
            'merkezi ve mağazalara götürmek yapabileceklerimden sadece '
            'birkaçı.',

            'Nöbetçi eczaneden kafeye, sinemadan benzin istasyonuna '
            'ihtiyacın olan yerleri ya da belirlediğin adresleri bulabilir, trafik '
            'durumunu ve gideceğimiz rotayı gösterebilir, hava durumunu '
            'söyleyebilirim. Ayrıca uzun uzun muhabbet etmeyi çok severim.',

            'Muhabbet etmekten seni istediğin yerlere götürmeye, trafik '
            'yoğunluğunu gösterip hava durumu bilgisi vermeye kadar uzun '
            'bir listem var.',
        ]

        response = alice('Ne yapabiliyorsun?')
        assert response.scenario == scenario.HandcraftedTr
        assert response.intent == intent.VinslessWhatCanYouDo
        first_reply = response.text
        assert first_reply in expected_response_texts

        response = alice('Başka ne yapabiliyorsun?')
        assert response.scenario == scenario.HandcraftedTr
        assert response.intent == intent.VinslessWhatCanYouDo
        second_reply = response.text
        assert second_reply in expected_response_texts

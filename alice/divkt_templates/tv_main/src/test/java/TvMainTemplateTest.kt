import org.junit.jupiter.api.Test
import ru.yandex.alice.divkttemplates.tvmain.TvMainTemplate
import ru.yandex.alice.protos.data.scenario.video.Gallery

class TvMainTemplateTest {
    @Test
    internal fun testTemplate() {
        TvMainTemplate.renderGallery(Gallery.TGalleryData.getDefaultInstance())
    }
}

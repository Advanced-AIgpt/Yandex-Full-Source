package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.renderer

import org.apache.logging.log4j.LogManager
import org.springframework.stereotype.Service
import ru.yandex.alice.kronstadt.core.DivRenderData
import ru.yandex.alice.paskill.dialogovo.external.v1.response.Image
import ru.yandex.alice.paskill.dialogovo.external.v1.response.audio.AudioMetadata
import ru.yandex.alice.paskill.dialogovo.external.v1.response.audio.Play
import ru.yandex.alice.protos.data.scenario.Data
import ru.yandex.alice.protos.data.scenario.music.Player
import ru.yandex.alice.protos.data.scenario.objects.MusicObjectsProto

@Service
class SkillAudioRendererService {

    fun getAudioPlayRenderData(playAction: Play): DivRenderData {
        logger.debug("building audio_play_render_data")
        return DivRenderData(
            skillAudioPlayerCardId,
            Data.TScenarioData.newBuilder()
                .setMusicPlayerData(
                    Player.TMusicPlayerData.newBuilder()
                        .setTrack(getTrack(playAction))
                        .build())
                .build()
        )
    }

    private fun getTrack(playAction: Play): MusicObjectsProto.TTrack {
        val title = playAction.audioItem.metadata
            .flatMap { obj: AudioMetadata -> obj.title }
        val builder = MusicObjectsProto.TTrack.newBuilder()
            .setAlbum(getAlbum(playAction))
            .addArtists(getArtist(playAction))
        if (title.isPresent) {
            builder.title = title.get()
        }
        return builder.build()

    }

    private fun getAlbum(playAction: Play): MusicObjectsProto.TAlbum {
        val image = playAction.audioItem.metadata
            .flatMap { obj: AudioMetadata -> obj.art }
            .map { obj: Image -> obj.url }
        val builder = MusicObjectsProto.TAlbum.newBuilder()
        if (image.isPresent) {
            builder.coverUri = image.get()
        }
        return builder.build()
    }

    private fun getArtist(playAction: Play): MusicObjectsProto.TArtist {
        val name = playAction.audioItem.metadata
            .flatMap { obj: AudioMetadata -> obj.subTitle }
        val builder = MusicObjectsProto.TArtist.newBuilder()
        if (name.isPresent) {
            builder.name = name.get()
        }
        return builder.build()
    }


    companion object {
        private val logger = LogManager.getLogger(SkillAudioRendererService::class)
        private const val skillAudioPlayerCardId = "skill.audio.player.div.card"
    }
}

package ru.yandex.alice.apphost

import com.fasterxml.jackson.annotation.JsonProperty
import com.fasterxml.jackson.databind.MapperFeature
import com.fasterxml.jackson.databind.ObjectMapper
import com.fasterxml.jackson.databind.SerializationFeature
import com.fasterxml.jackson.module.kotlin.KotlinModule
import org.apache.logging.log4j.LogManager
import java.io.File
import java.nio.file.Files
import java.nio.file.Paths

private val logger = LogManager.getLogger()

data class BackendPatch(
    val name: String,
    val port: Int,
    val host: String = "localhost",
    val ip: String = "[::1]",
)

private data class Instance(
    val ip: String = "[::1]",
    val host: String = "localhost",
    val port: Int,
    val weight: Int = 1,
    @JsonProperty("min_weight") val minWeight: Int = 1,
)

private data class ApphostBackendPatch(
    val instances: List<Instance>,
)

private fun generateBackends(
    backendsDir: String,
    prefix: String,
    devNullPort: Int,
    patches: Map<String, BackendPatch>,
): Map<String, ApphostBackendPatch> {
    val backends: MutableMap<String, ApphostBackendPatch> = mutableMapOf()
    for (file in Files.list(File(backendsDir).toPath())) {
        if (Files.isDirectory(file)) {
            backends.putAll(generateBackends(
                file.toString(),
                file.fileName.toString() + "__",
                devNullPort,
                emptyMap() // add patches only at top level
            ))
        } else if (Files.isRegularFile(file) && file.toString().endsWith(".json")) {
            backends[prefix + file.fileName.toString()] = ApphostBackendPatch(listOf(
                Instance(port = devNullPort)
            ))
        }
    }
    backends.putAll(
        patches.map {
            it.key to ApphostBackendPatch(listOf(Instance(port = it.value.port)))
        }.toMap()
    )
    return backends
}

private val DEFAULT_OBJECT_MAPPER = ObjectMapper()
    .configure(MapperFeature.SORT_PROPERTIES_ALPHABETICALLY, true)
    .configure(SerializationFeature.INDENT_OUTPUT, true)
    .registerModule(KotlinModule())

private data class BackendPatchFile(
    val backends: Map<String, ApphostBackendPatch>,
)

fun generateBackendsPatch(
    backendsDir: String,
    localApphostDir: String,
    devNullPort: Int,
    patches: Map<String, BackendPatch>,
    objectMapper: ObjectMapper = DEFAULT_OBJECT_MAPPER,
): String {
    val backends = generateBackends(backendsDir, "", devNullPort, patches)
    val backendPatchFilename = Paths.get(localApphostDir, "ALICE", "backends_patch.json")
    File(backendPatchFilename.parent.toString()).mkdirs()
    logger.info("Writing backend patches to {}", backendPatchFilename.toString())
    File(backendPatchFilename.toString()).printWriter().use { writer ->
        writer.append(objectMapper.writeValueAsString(BackendPatchFile(backends)))
    }
    return backendPatchFilename.toString()
}

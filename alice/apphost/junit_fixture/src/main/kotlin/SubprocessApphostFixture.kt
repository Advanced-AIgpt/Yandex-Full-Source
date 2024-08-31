import org.apache.logging.log4j.LogManager
import ru.yandex.alice.apphost.generateBackendsPatch
import ru.yandex.alice.apphost.pickRandomPortRange
import ru.yandex.alice.apphost.waitPort
import ru.yandex.devtools.test.Paths
import java.io.File
import java.time.Duration
import java.util.concurrent.TimeUnit

private fun String.appendPath(suffix: String): String {
    return if (this.endsWith(File.separator)) this + suffix else this + File.separator + suffix
}

internal class InvalidConfigurationException(
    override val message: String?
): Exception(message)

internal class SubprocessApphostFixture(
    private val tvmId: String = "2000860",
    private val vertical: String = "ALICE",
): ApphostFixture {
    private val config = Config.fromEnvironment()

    private val apphostBasePort: Int
    override val httpAdapterPort: Int

    private val apphostProcess: Process;

    init {
        apphostBasePort = pickRandomPortRange(range = 10)
        logger.info("Starting local apphost at base port {}", apphostBasePort)
        httpAdapterPort = apphostBasePort + 4
        val backendPatchPath = generateBackendsPatch(
                config.apphostBackendPath,
                config.localApphostDir,
                apphostBasePort + 5,
                emptyMap(),
        )
        val command = listOf(
            config.apphostBinaryPath,
            "setup",
            "--local-arcadia-path", config.arcadiaPath,
            "--port", apphostBasePort.toString(),
            "--force-yes",
            "--install-path", config.localApphostDir + "/",
            "--local-resolve",
            "--local-arcadia-path-for-binaries", config.binaryPath,
            "arcadia",
            "--vertical", vertical,
            "--target-ctype", "test",
            "--backends-patch-path", backendPatchPath,
            "--load-only-used-backends",
        )
        println("Starting apphost with command ${command.joinToString(" ")}")
        logger.info("Starting apphost with command {}", command.joinToString(" "))
        val processBuilder = ProcessBuilder(command)
            .redirectOutput(ProcessBuilder.Redirect.INHERIT)
            .redirectError(ProcessBuilder.Redirect.INHERIT)
        processBuilder.environment()["UNISTAT_DAEMON_PORT"] = (apphostBasePort + 7).toString()
        try {
            apphostProcess = processBuilder.start()
            waitPort(apphostProcess, "apphost", httpAdapterPort, timeout = Duration.ofMinutes(5))
        } catch (e: Exception) {
            close()
            throw e
        }
    }

    override fun close() {
        try {
            logger.info("Sending SIGINT to apphost launher process")
            Runtime.getRuntime().exec("kill -INT ${apphostProcess.pid()}")
            apphostProcess.waitFor(60, TimeUnit.SECONDS)
        } catch (e: Exception) {
            logger.error("Failed to gracefully stop apphost process, desqtroying", e)
            apphostProcess.destroy()
        }
    }

    companion object {
        private val logger = LogManager.getLogger();
    }
}

private data class Config private constructor(
    val arcadiaPath: String,
    val binaryPath: String,
    val testOutputPath: String,
) {

    val localApphostDir = testOutputPath.appendPath("local_app_host_dir")
    val apphostBinaryPath = binaryPath.appendPath("apphost/tools/app_host_launcher/app_host_launcher")
    val apphostBackendPath = arcadiaPath.appendPath("apphost/conf/backends")

    companion object {

        private val logger = LogManager.getLogger()

        fun fromEnvironment(): Config {
            if (Paths.getTestOutputsRoot() != null &&
                Paths.getSourcePath("") != "null/" &&
                Paths.getBuildPath("") != "null/"
            ) {
                // ya.make environment
                return Config(
                    Paths.getSourcePath(""),
                    Paths.getBuildPath(""),
                    Paths.getTestOutputsRoot(),
                )
            } else {
                // IDEA environment
                // TODO: validate that apphost is built
                val arcadiaPath = System.getenv("ARCADIA")
                if (arcadiaPath == null) {
                    throw InvalidConfigurationException(
                        """To run apphost tests from IntelliJ IDEA,
                            please set ${'$'}ARCADIA environment variable""".trimIndent()
                    )
                }
                return Config(
                    arcadiaPath,
                    arcadiaPath,
                    System.getProperty("user.dir"),
                )
            }
        }

    }
}

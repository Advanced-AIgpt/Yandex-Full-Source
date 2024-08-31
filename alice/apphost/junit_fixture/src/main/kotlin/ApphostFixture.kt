import ru.yandex.alice.apphost.ExternalApphostFixture
import ru.yandex.alice.apphost.checkPort

interface ApphostFixture {
    val httpAdapterPort: Int

    fun close()
}

internal class ApphostIsNotRunningException(port: Int): Exception("""
    Apphost HTTP adapter is not running at port $port. Run it with
    ya tool apphost setup --local-arcadia-path ${"$"}ARCADIA
    --port 9000 --force-yes --tvm-id 2000860 arcadia
    --vertical ALICE --target-ctype test
""".trimIndent().replace("\n", " "))

fun createApphostFixture(): ApphostFixture {
    val localHttpAdapterPort = System.getenv("APPHOST_FIXTURE_HTTP_ADAPTER_PORT")?.toInt()
    if (localHttpAdapterPort == null) {
        // TODO: pass args
        return SubprocessApphostFixture()
    } else {
        if (!checkPort("localhost", localHttpAdapterPort)) {
            throw ApphostIsNotRunningException(localHttpAdapterPort)
        }
        return ExternalApphostFixture(localHttpAdapterPort)
    }
}

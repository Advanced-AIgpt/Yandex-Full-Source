package ru.yandex.alice.kronstadt.core.db

import java.util.Random
import java.util.concurrent.ThreadLocalRandom

@JvmOverloads
fun reorderHosts(
    portoHost: String?,
    hosts: String,
    r: Random = ThreadLocalRandom.current()
): String {
    val hostsList =
        mutableListOf(*hosts.split(",".toRegex()).toTypedArray())
    if (hostsList.size > 1 && portoHost != null && portoHost.length > 3) {
        val location = portoHost.substring(0, 3)
        hostsList.shuffle(r)
        var index = -1
        for (i in hostsList.indices) {
            if (hostsList[i].startsWith(location)) {
                index = i
                break
            }
        }
        if (index > 0) {
            val tmp = hostsList[0]
            hostsList[0] = hostsList[index]
            hostsList[index] = tmp
        }
    }
    return hostsList.joinToString(separator = ",")
}

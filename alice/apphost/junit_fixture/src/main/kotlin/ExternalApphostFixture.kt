package ru.yandex.alice.apphost

import ApphostFixture

internal class ExternalApphostFixture(
    override val httpAdapterPort: Int
): ApphostFixture {

    override fun close() {
    }

}

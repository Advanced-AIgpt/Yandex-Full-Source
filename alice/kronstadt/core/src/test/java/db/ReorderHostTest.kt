package ru.yandex.alice.kronstadt.core.db

import org.junit.jupiter.api.Assertions.assertEquals
import org.junit.jupiter.api.BeforeEach
import org.junit.jupiter.api.Test
import java.util.Random

internal class ReorderHostTest {
    private lateinit var r: Random

    @BeforeEach
    fun setUp() {
        r = Random(2L)
    }

    @Test
    fun anotherHost() {
        val hosts = "vla-6p6gg0zklgfpliia.db.yandex.net:6432,man-d84liusfqlj7tn0c.db.yandex.net:6432," +
            "sas-4mvide6opucq9bhg.db.yandex.net:6432"
        val portoHost = "sas3-8556.search.yandex.net"
        assertEquals(
            "sas-4mvide6opucq9bhg.db.yandex.net:6432,vla-6p6gg0zklgfpliia.db.yandex.net:6432,man-d84liusfqlj7tn0c" +
                ".db.yandex.net:6432",
            reorderHosts(portoHost, hosts, r)
        )
    }

    @Test
    fun sameHost() {
        val hosts = "vla-6p6gg0zklgfpliia.db.yandex.net:6432,man-d84liusfqlj7tn0c.db.yandex.net:6432," +
            "sas-4mvide6opucq9bhg.db.yandex.net:6432"
        val portoHost = "vla3-8556.search.yandex.net"
        assertEquals(
            "vla-6p6gg0zklgfpliia.db.yandex.net:6432,sas-4mvide6opucq9bhg.db.yandex.net:6432,man-d84liusfqlj7tn0c" +
                ".db.yandex.net:6432",
            reorderHosts(portoHost, hosts, r)
        )
    }

    @Test
    fun missingHost() {
        val hosts = "vla-6p6gg0zklgfpliia.db.yandex.net:6432,man-d84liusfqlj7tn0c.db.yandex.net:6432," +
            "sas-4mvide6opucq9bhg.db.yandex.net:6432"
        val portoHost = "iva3-8556.search.yandex.net"
        assertEquals(
            "sas-4mvide6opucq9bhg.db.yandex.net:6432,vla-6p6gg0zklgfpliia.db.yandex.net:6432,man-d84liusfqlj7tn0c" +
                ".db.yandex.net:6432",
            reorderHosts(portoHost, hosts, r)
        )
    }

    @Test
    fun singlePgHost() {
        val hosts = "vla-6p6gg0zklgfpliia.db.yandex.net:6432"
        val portoHost = "sas3-8556.search.yandex.net"
        assertEquals(
            "vla-6p6gg0zklgfpliia.db.yandex.net:6432",
            reorderHosts(portoHost, hosts, r)
        )
    }

    @Test
    fun nullProtoHost() {
        val hosts = "vla-6p6gg0zklgfpliia.db.yandex.net:6432,man-d84liusfqlj7tn0c.db.yandex.net:6432," +
            "sas-4mvide6opucq9bhg.db.yandex.net:6432"
        assertEquals(
            "vla-6p6gg0zklgfpliia.db.yandex.net:6432,man-d84liusfqlj7tn0c.db.yandex.net:6432,sas-4mvide6opucq9bhg" +
                ".db.yandex.net:6432",
            reorderHosts(null, hosts, r)
        )
    }
}

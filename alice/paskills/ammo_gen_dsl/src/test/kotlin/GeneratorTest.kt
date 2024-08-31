package ru.yandex.alice.paskills.ammo

import com.google.protobuf.Any
import org.junit.jupiter.api.Assertions.assertEquals
import org.junit.jupiter.api.Test
import java.io.ByteArrayOutputStream
import java.nio.charset.StandardCharsets

class GeneratorTest {

    @Test
    fun `get without headers`() {
        val actual = with(ByteArrayOutputStream()) {
            generateToOutputStream(output = this) {
                get(path = "/") {
                    this
                }
            }
            String(this.toByteArray())
        }
        val bullet = "GET / HTTP/1.1\r\n\r\n"
        val expected = "${bullet.length} \n$bullet\n"
        assertEquals(18, bullet.toByteArray().size)
        assertEquals(expected, actual)
    }

    @Test
    fun `get with headers`() {
        val actual = with(ByteArrayOutputStream()) {
            generateToOutputStream(output = this) {
                get(path = "/") {
                    header("Host", "example.com")
                    version(Version.HTTP_1_0)
                }
            }
            String(this.toByteArray())
        }
        val bullet = "GET / HTTP/1.0\r\n" +
            "Host: example.com\r\n" +
            "\r\n"
        val expected = "${bullet.length} \n$bullet\n"
        assertEquals(37, bullet.toByteArray().size)
        assertEquals(expected, actual)
    }

    @Test
    fun `post with body with explicit content length`() {

        val actual = with(ByteArrayOutputStream()) {
            generateToOutputStream(output = this) {
                post(path = "/", tag = "simple_post") {
                    headers(
                        "Host" to "example.com",
                        "Accept" to "*/*",
                        "Content-Length" to "7",
                        "Content-Type" to "application/x-www-form-urlencoded"
                    )
                    body("foo=bar")
                }
            }
            String(this.toByteArray())
        }

        val bullet = "POST / HTTP/1.1\r\n" +
            "Host: example.com\r\n" +
            "Accept: */*\r\n" +
            "Content-Length: 7\r\n" +
            "Content-Type: application/x-www-form-urlencoded\r\n" +
            "\r\n" +
            "foo=bar"
        val expected = "126 simple_post\n$bullet\n"

        assertEquals(126, bullet.toByteArray().size)
        assertEquals(expected, actual)
    }

    @Test
    fun `post with body`() {
        val actual = with(ByteArrayOutputStream()) {
            generateToOutputStream(output = this) {
                post(path = "/", tag = "simple_post") {
                    headers(
                        "Host" to "example.com",
                        "Accept" to "*/*",
                        "Content-Type" to "application/x-www-form-urlencoded"
                    )
                    body("foo=bar")
                }
            }
            String(this.toByteArray())
        }

        val bullet = "POST / HTTP/1.1\r\n" +
            "Host: example.com\r\n" +
            "Accept: */*\r\n" +
            "Content-Type: application/x-www-form-urlencoded\r\n" +
            "Content-Length: 7\r\n" +
            "\r\n" +
            "foo=bar"
        val expected = "126 simple_post\n$bullet\n"

        assertEquals(126, bullet.toByteArray().size)
        assertEquals(expected, actual)
    }

    @Test
    fun `post with protobuf body`() {
        val payload = Any.newBuilder().setTypeUrl("testurl").build()
        val actual = with(ByteArrayOutputStream()) {
            generateToOutputStream(output = this) {
                post(path = "/", tag = "simple_post") {
                    headers(
                        "Host" to "example.com",
                        "Accept" to "*/*",
                        "Content-Type" to "application/x-www-form-urlencoded"
                    )
                    body(payload)
                }
            }
            this.toByteArray()
        }

        val bullet = "POST / HTTP/1.1\r\n" +
            "Host: example.com\r\n" +
            "Accept: */*\r\n" +
            "Content-Type: application/x-www-form-urlencoded\r\n" +
            "Content-Length: ${payload.toByteArray().size}\r\n" +
            "\r\n" +
            String(payload.toByteArray(), StandardCharsets.UTF_8)

        val expected = "${bullet.toByteArray().size} simple_post\n$bullet\n"
        assertEquals(expected, String(actual, StandardCharsets.UTF_8))
        assertEquals(128, bullet.toByteArray().size)
    }

    @Test
    fun `post with protobuf body to stdout`() {
        val payload = Any.newBuilder().setTypeUrl("testurl").build()
        generateToStdOut {
            post(path = "/", tag = "simple_post") {
                headers(
                    "Host" to "example.com",
                    "Accept" to "*/*",
                    "Content-Type" to "application/x-www-form-urlencoded"
                )
                body(payload)
            }
        }
    }
}

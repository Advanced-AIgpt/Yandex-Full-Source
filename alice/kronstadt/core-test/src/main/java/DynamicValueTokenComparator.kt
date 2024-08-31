package ru.yandex.alice.kronstadt.test

import org.json.JSONException
import org.skyscreamer.jsonassert.JSONCompareMode
import org.skyscreamer.jsonassert.JSONCompareResult
import org.skyscreamer.jsonassert.comparator.DefaultComparator
import java.net.URI
import java.util.UUID

class DynamicValueTokenComparator(mode: JSONCompareMode) : DefaultComparator(mode) {

    @Throws(JSONException::class)
    override fun compareValues(
        prefix: String,
        expectedValue: Any,
        actualValue: Any,
        jsonCompareResult: JSONCompareResult
    ) {
        if (expectedValue.toString() == "<UUID>") {
            try {
                UUID.fromString(actualValue.toString())
            } catch (e: IllegalArgumentException) {
                throw JSONException("Invalid UUID for field $prefix")
            }
            return
        } else if (expectedValue.toString() == "<TIMESTAMP>") {
            if (actualValue.toString().isNotEmpty()) {
                try {
                    java.lang.Long.parseUnsignedLong(actualValue.toString())
                    jsonCompareResult.passed()
                    return
                } catch (e: NumberFormatException) {
                    throw JSONException("Invalid timestamp for field $prefix")
                }
            } else {
                throw JSONException("Missing timestamp value for field $prefix")
            }
        } else if (expectedValue.toString() == "<UINT>") {
            if (actualValue.toString().isNotEmpty()) {
                try {
                    java.lang.Long.parseUnsignedLong(actualValue.toString())
                    jsonCompareResult.passed()
                    return
                } catch (e: NumberFormatException) {
                    throw JSONException("Invalid uint for field $prefix")
                }
            } else {
                throw JSONException("Missing uint value for field $prefix")
            }
        } else if (expectedValue.toString() == "<URL>") {
            if (actualValue.toString().isNotEmpty()) {
                try {
                    URI.create(actualValue.toString())
                    jsonCompareResult.passed()
                    return
                } catch (e: NumberFormatException) {
                    throw JSONException("Invalid URI for field $prefix")
                }
            } else {
                throw JSONException("Missing URI value for field $prefix")
            }
        }
        super.compareValues(prefix, expectedValue, actualValue, jsonCompareResult)
    }

    companion object {
        val STRICT = DynamicValueTokenComparator(JSONCompareMode.STRICT)
    }
}

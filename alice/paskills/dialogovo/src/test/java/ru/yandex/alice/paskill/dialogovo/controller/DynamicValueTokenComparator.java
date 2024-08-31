package ru.yandex.alice.paskill.dialogovo.controller;

import java.net.URI;
import java.util.UUID;

import org.json.JSONException;
import org.skyscreamer.jsonassert.JSONCompareMode;
import org.skyscreamer.jsonassert.JSONCompareResult;
import org.skyscreamer.jsonassert.comparator.DefaultComparator;

class DynamicValueTokenComparator extends DefaultComparator {

    DynamicValueTokenComparator(JSONCompareMode mode) {
        super(mode);
    }

    @Override
    public void compareValues(
            String prefix,
            Object expectedValue,
            Object actualValue,
            JSONCompareResult jsonCompareResult
    ) throws JSONException {
        if (expectedValue.toString().equals("<UUID>")) {
            try {
                UUID.fromString(actualValue.toString());
            } catch (IllegalArgumentException e) {
                throw new JSONException("Invalid UUID for field " + prefix);
            }

            return;
        } else if (expectedValue.toString().equals("<TIMESTAMP>")) {
            if (!actualValue.toString().isEmpty()) {
                try {
                    Long.parseUnsignedLong(actualValue.toString());
                    jsonCompareResult.passed();
                    return;
                } catch (NumberFormatException e) {
                    throw new JSONException("Invalid timestamp for field " + prefix);
                }
            } else {
                throw new JSONException("Missing timestamp value for field " + prefix);
            }
        } else if (expectedValue.toString().equals("<UINT>")) {
            if (!actualValue.toString().isEmpty()) {
                try {
                    Long.parseUnsignedLong(actualValue.toString());
                    jsonCompareResult.passed();
                    return;
                } catch (NumberFormatException e) {
                    throw new JSONException("Invalid uint for field " + prefix);
                }
            } else {
                throw new JSONException("Missing uint value for field " + prefix);
            }
        } else if (expectedValue.toString().equals("<URL>")) {
            if (!actualValue.toString().isEmpty()) {
                try {
                    URI.create(actualValue.toString());
                    jsonCompareResult.passed();
                    return;
                } catch (NumberFormatException e) {
                    throw new JSONException("Invalid URI for field " + prefix);
                }
            } else {
                throw new JSONException("Missing URI value for field " + prefix);
            }
        }

        super.compareValues(prefix, expectedValue, actualValue, jsonCompareResult);
    }
}

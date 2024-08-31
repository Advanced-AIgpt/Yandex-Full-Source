package ru.yandex.alice.paskill.dialogovo.test.utils;

import java.util.UUID;

import org.json.JSONException;
import org.skyscreamer.jsonassert.JSONCompareMode;
import org.skyscreamer.jsonassert.JSONCompareResult;
import org.skyscreamer.jsonassert.comparator.DefaultComparator;

public class DynamicValueTokenComparator extends DefaultComparator {

    public DynamicValueTokenComparator(JSONCompareMode mode) {
        super(mode);
    }

    @Override
    public void compareValues(
            String s,
            Object expectedValue,
            Object actualValue,
            JSONCompareResult jsonCompareResult
    ) throws JSONException {
        if (expectedValue.toString().equals("<UUID>")) {
            try {
                UUID.fromString(actualValue.toString());
            } catch (IllegalArgumentException e) {
                throw new JSONException("Invalid UUID for field " + s);
            }

            return;
        } else if (expectedValue.toString().equals("<TIMESTAMP>")) {
            if (!actualValue.toString().isEmpty()) {
                try {
                    Long.parseUnsignedLong(actualValue.toString());
                    jsonCompareResult.passed();
                    return;
                } catch (NumberFormatException e) {
                    throw new JSONException("Invalid timestamp for field " + s);
                }
            } else {
                throw new JSONException("Missing timestamp value for field " + s);
            }
        }

        super.compareValues(s, expectedValue, actualValue, jsonCompareResult);
    }
}

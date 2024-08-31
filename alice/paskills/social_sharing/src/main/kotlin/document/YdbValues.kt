package ru.yandex.alice.social.sharing.document

import com.yandex.ydb.table.values.OptionalType
import com.yandex.ydb.table.values.OptionalValue
import com.yandex.ydb.table.values.PrimitiveType
import com.yandex.ydb.table.values.PrimitiveValue
import java.time.Instant

private val optionalUtf8: OptionalType = OptionalType.of(PrimitiveType.utf8())
private val optionalString: OptionalType = OptionalType.of(PrimitiveType.string())
private val optionalUInt32: OptionalType = OptionalType.of(PrimitiveType.uint32())

internal fun String.toUtf8(): PrimitiveValue {
    return PrimitiveValue.utf8(this)
}

internal fun Instant.toTimestamp(): PrimitiveValue {
    return PrimitiveValue.timestamp(this)
}

internal fun String?.toUtf8Opt(): OptionalValue {
    return if (this.isNullOrEmpty()) optionalUtf8.emptyValue() else optionalUtf8.newValue(this.toUtf8())
}

internal fun Int.toUint32(): PrimitiveValue {
    return PrimitiveValue.uint32(this)
}

internal fun Long.toUint64(): PrimitiveValue {
    return PrimitiveValue.uint64(this)
}

internal fun Int?.toUint32Opt(): OptionalValue {
    return if (this == null || this == 0) optionalUInt32.emptyValue() else optionalUInt32.newValue(this.toUint32())
}

internal fun ByteArray.toYdbString(): PrimitiveValue {
    return PrimitiveValue.string(this)
}

internal fun ByteArray?.toYdbStringOpt(): OptionalValue {
    return if (this == null) optionalString.emptyValue() else optionalString.newValue(this.toYdbString())
}

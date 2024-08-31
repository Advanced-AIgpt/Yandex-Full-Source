package ru.yandex.alice.kronstadt.test

import com.fasterxml.jackson.databind.JsonNode
import ru.yandex.alice.megamind.protos.scenarios.ResponseProto.TScenarioRunResponse

enum class SideEffectRequestType(private val type: String?) {
    APPLY("apply"),
    COMMIT("commit"),
    CONTINUE("continue"),
    RUN_ONLY(null),
    IRRELEVANT(null);

    fun hasSideEffects(): Boolean {
        return this == APPLY || this == COMMIT || this == CONTINUE
    }

    fun getType(): String {
        if (type == null) {
            throw RuntimeException("Only responses with side effects can have a type")
        }
        return type
    }

    val mockResponseFilename: String
        get() = getType() + "_response.json"

    fun getUrl(port: Int, baseUrl: String): String {
        return "http://localhost:" + port + baseUrl + "/" + getType()
    }

    fun getApplyArguments(runResponse: JsonNode): JsonNode {
        return when (this) {
            APPLY -> runResponse["apply_arguments"]
            COMMIT -> runResponse["commit_candidate"]["arguments"]
            CONTINUE -> runResponse["continue_arguments"]
            else -> throw RuntimeException("Only apply, commit and continue requests can have apply arguments")
        }
    }

    companion object {
        fun fromRunResponse(runResponse: TScenarioRunResponse): SideEffectRequestType {
            return when {
                runResponse.features.isIrrelevant -> IRRELEVANT
                runResponse.hasApplyArguments() -> APPLY
                runResponse.hasCommitCandidate() -> COMMIT
                runResponse.hasContinueArguments() -> CONTINUE
                else -> RUN_ONLY
            }
        }
    }
}

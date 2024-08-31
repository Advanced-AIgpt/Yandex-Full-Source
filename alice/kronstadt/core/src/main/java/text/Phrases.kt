package ru.yandex.alice.kronstadt.core.text

import org.apache.logging.log4j.LogManager
import ru.yandex.alice.kronstadt.core.layout.TextWithTts
import ru.yandex.alice.kronstadt.core.text.InflectedString.RuCase
import ru.yandex.alice.kronstadt.core.text.Language.Companion.all
import ru.yandex.alice.kronstadt.core.text.nlg.TemplateToTextConverter.stripMarkup
import ru.yandex.alice.kronstadt.core.text.nlg.TemplateToTtsConverter.ttsify
import java.util.Optional
import java.util.Random
import java.util.ResourceBundle
import java.util.regex.Matcher
import java.util.regex.Pattern

class Phrases(resourceBundles: List<String>) {
    private val phrases: Map<Language, MutableMap<String, String>>
    private val phrasePools: Map<Language, MutableMap<String, MutableList<String>>>

    init {
        val phrasesTmp: MutableMap<Language, MutableMap<String, String>> = hashMapOf()
        val phrasePoolsTmp: MutableMap<Language, MutableMap<String, MutableList<String>>> = hashMapOf()
        for (resourceBundle in resourceBundles) {
            for (language in all()) {
                val bundle = ResourceBundle.getBundle(resourceBundle, language.locale)
                val en = bundle.keys
                while (en.hasMoreElements()) {
                    val key = en.nextElement()
                    val value = bundle.getString(key)
                    val phraseKeys = phrasesTmp.computeIfAbsent(language) { hashMapOf() }
                    phraseKeys[key] = value
                    phrasesTmp[language] = phraseKeys
                    val phrasePoolKeys = phrasePoolsTmp.computeIfAbsent(language) { hashMapOf() }
                    val subs = key.split("\\.".toRegex()).dropLastWhile { it.isEmpty() }.toTypedArray()
                    if (subs.size > 1) {
                        val parts = subs.sliceArray(0 until subs.size - 1).joinToString(separator = ".")
                        val phrasePoolList = phrasePoolKeys.computeIfAbsent(parts) { mutableListOf() }
                        phrasePoolList.add(value)
                    }
                }
            }
        }
        phrases = phrasesTmp.toMap()
        phrasePools = phrasePoolsTmp.toMap()
    }

    fun getTextWithTts(
        key: String,
        textVariables: List<String>,
        ttsVariables: List<String>
    ): TextWithTts {
        val template = get(key, "")
        val text = stripMarkup(template)
        val tts = ttsify(template)
        return TextWithTts(
            render(text, textVariables),
            render(tts, ttsVariables)
        )
    }

    fun getTextWithTts(key: String, vararg variables: TextWithTts): TextWithTts {
        val textVariables: MutableList<String> = ArrayList()
        val ttsVariables: MutableList<String> = ArrayList()
        for (variable in variables) {
            textVariables.add(variable.text)
            ttsVariables.add(variable.tts)
        }
        return getTextWithTts(key, textVariables, ttsVariables)
    }

    operator fun get(key: String, defaultText: String, vararg variables: String): String {
        return render(get(key, defaultText, Language.RUSSIAN), *variables)
    }

    operator fun get(key: String, variables: List<String>): String {
        return render(get(key, "", Language.RUSSIAN), variables)
    }

    operator fun get(key: String, vararg variables: InflectedString): String {
        return render(get(key, "", Language.RUSSIAN), *variables)
    }

    @JvmOverloads
    operator fun get(key: String, defaultText: String = "", language: Language = Language.RUSSIAN): String {
        return getO(key, language)
            .orElse(defaultText)
    }

    private fun getO(key: String, language: Language): Optional<String> {
        return Optional.ofNullable<Map<String, String>>(phrases[language])
            .flatMap { langTexts: Map<String, String> ->
                Optional.ofNullable(
                    langTexts[key]
                )
            }
            .or {
                Optional.ofNullable<Map<String, String>>(phrases[Language.RUSSIAN])
                    .flatMap { langTexts: Map<String, String> ->
                        Optional.ofNullable(
                            langTexts[key]
                        )
                    }
            }
    }

    fun getAll(key: String, vararg variables: String): List<String> {
        return getPool(key, "{0}")
            .map { template: String -> render(template, *variables) }
    }

    fun getRandom(key: String, defaultText: String, random: Random): String {
        val pool = getPool(key, Language.RUSSIAN, defaultText)
        return pool[random.nextInt(pool.size)]
    }

    fun getRandom(key: String, defaultText: String, random: Random, language: Language): String {
        val pool = getPool(key, language, defaultText)
        return pool[random.nextInt(pool.size)]
    }

    fun getRandom(key: String, defaultText: String, random: Random, vararg variables: String): String {
        return render(getRandom(key, random, defaultText), *variables)
    }

    fun getRandom(key: String, random: Random, vararg variables: String): String {
        return render(getRandom(key, "", random), *variables)
    }

    fun getRandom(key: String, random: Random): String {
        return render(getRandom(key, "", random), listOf())
    }

    fun getRandom(key: String, random: Random, vararg variables: InflectedString): String {
        return render(getRandom(key, "", random), *variables)
    }

    fun getRandomTextWithTts(
        key: String,
        random: Random,
        textVariables: List<String>,
        ttsVariables: List<String>
    ): TextWithTts {
        val template = getRandom(key, random)
        val ttsTemplate = ttsify(template)
        val textTemplate = stripMarkup(template)
        return TextWithTts(
            render(textTemplate, textVariables),
            render(ttsTemplate, ttsVariables)
        )
    }

    fun getRandomTextWithTts(key: String, random: Random, vararg variables: TextWithTts): TextWithTts {
        val textVars: MutableList<String> = ArrayList(variables.size)
        val ttsVars: MutableList<String> = ArrayList(variables.size)
        for (variable in variables) {
            textVars.add(variable.text)
            ttsVars.add(variable.tts)
        }
        return getRandomTextWithTts(key, random, textVars, ttsVars)
    }

    fun render(template: String, vararg variables: String): String {
        return render(template, listOf(*variables))
    }

    private fun render(template: String, variables: List<String>): String {
        var result = template
        for (i in variables.indices) {

            // The $ is group symbol in regex's replacement parameter. It has to be escaped
            val replacement = variables[i].replace("$", "\\$")
            result = PATTERNS[i].matcher(result).replaceAll(replacement)
        }
        return result
    }

    fun render(template: String, vararg variables: InflectedString): String {
        return renderInflected(template, listOf(*variables))
    }

    private fun renderInflected(template: String, variables: List<InflectedString>): String {
        var result = template
        for (i in variables.indices) {
            val templateMatcher = PATTERNS[i].matcher(result)
            // can be multiple links to same variable {0} in one phrase
            while (templateMatcher.find()) {
                val templateGroup = getMatcherGroupSafe(templateMatcher, null)
                if (templateGroup != null) {
                    var ruCase: RuCase? = RuCase.NOMINATIVE
                    val caseMatcher = CASE_PATTERNS[i].matcher(templateGroup)
                    if (caseMatcher.find()) {
                        val caseGroup = getMatcherGroupSafe(caseMatcher, "case")
                        if (caseGroup != null) {
                            ruCase = RuCase.R.fromValueOrDefault(caseGroup, ruCase)
                        }
                    }
                    result = result.replace(templateGroup, variables[i][ruCase!!])
                }
            }
        }
        return result
    }

    private fun getMatcherGroupSafe(matcher: Matcher, groupName: String?): String? {
        try {
            return if (groupName != null) matcher.group(groupName) else matcher.group()
        } catch (ex: Exception) {
            logger.error("Error while capturing inflected group", ex)
        }
        return null
    }

    fun getPool(key: String, defaultText: String): List<String> {
        return getPool(key, Language.RUSSIAN, defaultText)
    }

    fun getPool(key: String, language: Language, defaultText: String): List<String> {
        return getPoolO(key, language).orElseGet { listOf(defaultText) }
    }

    fun getPoolO(key: String, language: Language): Optional<List<String>> {
        return Optional.ofNullable<Map<String, MutableList<String>>>(
            phrasePools[language]
        )
            .flatMap { langTexts: Map<String, MutableList<String>> ->
                Optional.ofNullable<List<String>>(
                    langTexts[key]
                )
            }
            .or {
                Optional.ofNullable<Map<String, MutableList<String>>>(
                    phrasePools[Language.RUSSIAN]
                )
                    .flatMap { langTexts: Map<String, MutableList<String>> ->
                        Optional.ofNullable<List<String>>(
                            langTexts[key]
                        )
                    }
            }
    }

    companion object {
        private val logger = LogManager.getLogger()
        private val PATTERNS: Array<Pattern> = (0 until 20)
            .map { Pattern.compile("\\{$it[:\\w]*}") }
            .toTypedArray()

        private val CASE_PATTERNS: Array<Pattern> = (0 until 20)
            .map { Pattern.compile("\\{$it:case:(?<case>\\w+)}") }
            .toTypedArray()
    }
}

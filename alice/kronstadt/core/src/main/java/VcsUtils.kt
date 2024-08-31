package ru.yandex.alice.kronstadt.core

import org.springframework.stereotype.Component
import java.io.IOException
import java.util.jar.Manifest

@Component
open class VcsUtils internal constructor() : VersionProvider {
    override val version: String
    override val branch: String
    override val tag: String

    init {
        var ver: String? = null
        var branchValue: String? = null
        var tagValue: String? = null
        try {
            val resources = javaClass.classLoader.getResources("META-INF/MANIFEST.MF")
            while (resources.hasMoreElements()) {
                val manifest = Manifest(resources.nextElement().openStream())
                val attributes = manifest.mainAttributes
                if (!attributes.getValue("Arcadia-Source-Last-Change").isNullOrEmpty()) {
                    ver = attributes.getValue("Arcadia-Source-Last-Change")
                }
                if (!attributes.getValue("Branch").isNullOrEmpty()) {
                    branchValue = attributes.getValue("Branch")
                }
                if (!attributes.getValue("Arcadia-Tag").isNullOrEmpty()) {
                    tagValue = attributes.getValue("Arcadia-Tag")
                }
            }
        } catch (ignored: IOException) {
        }
        this.version = ver ?: ""
        this.branch = branchValue ?: ""
        this.tag = tagValue ?: ""
    }
}

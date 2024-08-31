package ru.yandex.alice.kronstadt.generator

import NAppHostPbConfig.NNora.Graph
import NAppHostPbConfig.NNora.Shared
import com.google.protobuf.util.JsonFormat
import ru.yandex.alice.kronstadt.tools.generator.TGraphWrapper
import java.io.File

internal class Merger {

    fun mergeGraph(
        currentGraphFile: File,
        baseGraph: TGraphWrapper
    ): TGraphWrapper {
        return if (!currentGraphFile.exists()) {
            baseGraph
        } else {
            val builder = baseGraph.toBuilder()
            val current: TGraphWrapper = parseGraph(currentGraphFile).build()

            if (builder.hasMonitoring() || current.hasMonitoring()) {
                mergeDefaultNodeAlerts(builder.monitoringBuilder, current.monitoring)
            }

            mergeSettings(builder.settingsBuilder, current.settings)

            builder.build()
        }
    }

    private fun parseGraph(graphFile: File): TGraphWrapper.Builder {
        val parser = JsonFormat.parser()
        val graphConfigBuilder = TGraphWrapper.newBuilder()

        graphFile.reader().use {
            parser.merge(it, graphConfigBuilder)
        }

        return graphConfigBuilder
    }

    private fun mergeSettings(settingsBuilder: Graph.TGraph.Builder, settings: Graph.TGraph) {
        // base expressions are must!
        settingsBuilder.putAllExpressions(settings.expressionsMap + settingsBuilder.expressionsMap)
        settingsBuilder.putAllEdgeExpressions(settings.edgeExpressionsMap + settingsBuilder.edgeExpressionsMap)

        val nodeDeps = settingsBuilder.nodeDepsMap.toMutableMap()
        settings.nodeDepsMap.forEach { (key, deps) ->
            nodeDeps.merge(key, deps) { old, new ->
                old.toBuilder().addAllInputDeps(new.inputDepsList - old.inputDepsList).build()
            }
        }
        settingsBuilder.putAllNodeDeps(nodeDeps)

        settingsBuilder.addAllInputDeps(settings.inputDepsList - settingsBuilder.inputDepsList)
        settingsBuilder.addAllOutputDeps(settings.outputDepsList - settingsBuilder.outputDepsList)
        settingsBuilder.addAllCriticalDeps(settings.criticalDepsList - settingsBuilder.criticalDepsList)
        val nodes = settingsBuilder.nodesMap.toMutableMap()
        settings.nodesMap.forEach { (name, node) ->
            nodes.merge(name, node) { old, _ -> old }
        }

        settingsBuilder.putAllNodes(settings.nodesMap - settingsBuilder.nodesMap.keys)

        if (settingsBuilder.hasAliasConfig() || settings.hasAliasConfig()) {
            mergeAliasConfig(settingsBuilder.aliasConfigBuilder, settings.aliasConfig)
        }

        if (settings.hasDumpInputProbability()) {
            settingsBuilder.dumpInputProbability = settings.dumpInputProbability
        }
        if (settings.hasQuotaGroup()) {
            settingsBuilder.quotaGroup = settings.quotaGroup
        }
        if (settingsBuilder.streamingNoBlockOutputs || settings.streamingNoBlockOutputs) {
            settingsBuilder.streamingNoBlockOutputs = true
        }
        val nodeSubsets = settingsBuilder.nodeSubsetsMap.toMutableMap()

        settings.nodeSubsetsMap.forEach { (key, items) ->
            nodeSubsets.merge(key, items) { old, new ->
                old.toBuilder().addAllMembers(new.membersList - old.membersList).build()
            }
        }
        settingsBuilder.putAllNodeSubsets(nodeSubsets)

        if (settingsBuilder.hasResponsibles() || settings.hasResponsibles()) {
            mergeResponsibles(settingsBuilder.responsiblesBuilder, settings.responsibles)
        }

        settingsBuilder.addAllAllowedTvmIds(settings.allowedTvmIdsList - settingsBuilder.allowedTvmIdsList)
        if (settingsBuilder.allowEmptyResponse || settings.allowEmptyResponse) {
            settingsBuilder.allowEmptyResponse = true
        }
        if (settingsBuilder.hasDefaultNodeAlerts() || settings.hasDefaultNodeAlerts()) {
            mergeDefaultNodeAlerts(settingsBuilder.defaultNodeAlertsBuilder, settings.defaultNodeAlerts)
        }
    }

    private fun mergeResponsibles(
        responsiblesBuilder: Shared.TResponsibles.Builder,
        responsibles: Shared.TResponsibles
    ) {
        responsiblesBuilder.addAllLogins(responsibles.loginsList - responsiblesBuilder.loginsList)
        responsiblesBuilder.addAllAbc(responsibles.abcList - responsiblesBuilder.abcList)
        responsiblesBuilder.addAllStaffDepartments(responsibles.staffDepartmentsList - responsiblesBuilder.staffDepartmentsList)
        responsiblesBuilder.addAllNotificationGroups(responsibles.notificationGroupsList - responsiblesBuilder.notificationGroupsList)
        responsiblesBuilder.addAllMessengerChatNames(responsibles.messengerChatNamesList - responsiblesBuilder.messengerChatNamesList)
        responsiblesBuilder.addAllAbcService(responsibles.abcServiceList - responsiblesBuilder.abcServiceList.toSet())
    }

    private fun mergeAliasConfig(aliasConfigBuilder: Graph.TAliasConfig.Builder, aliasConfig: Graph.TAliasConfig) {
        aliasConfigBuilder.addAllAskAlias(aliasConfig.askAliasList - aliasConfigBuilder.askAliasList)
        aliasConfigBuilder.addAllParamAlias(aliasConfig.paramAliasList - aliasConfigBuilder.paramAliasList)
        aliasConfigBuilder.addAllSkipAlias(aliasConfig.skipAliasList - aliasConfigBuilder.skipAliasList)
        aliasConfigBuilder.addAllAddrAlias(aliasConfig.addrAliasList - aliasConfigBuilder.addrAliasList)
        aliasConfigBuilder.addAllStatAlias(aliasConfig.statAliasList - aliasConfigBuilder.statAliasList)
    }

    private fun mergeDefaultNodeAlerts(
        monitoringBuilder: Graph.TDefaultNodeAlerts.Builder,
        monitoring: Graph.TDefaultNodeAlerts
    ) {
        monitoringBuilder.addAllAlerts(monitoring.alertsList - monitoringBuilder.alertsList)
    }
}

package ru.yandex.qe.hitman.comrade.script.execution.groovy.example

import ru.yandex.qe.hitman.comrade.script.model.ComradeClient
import ru.yandex.qe.hitman.comrade.script.model.EventListener
import ru.yandex.qe.hitman.comrade.script.model.Storage
import ru.yandex.qe.hitman.comrade.script.model.toloka.Assignment
import ru.yandex.qe.hitman.comrade.script.model.toloka.AssignmentStatus
import ru.yandex.qe.hitman.comrade.script.util.Answer
import ru.yandex.qe.hitman.comrade.script.util.ComradeUtils
import ru.yandex.qe.hitman.comrade.script.util.collections.Counters
import ru.yandex.qe.hitman.comrade.script.util.hpprocessor.HpProcessor

/**
*  User: artpetroff
*  Date: 01/11/16
* Updated 14/12/17 by yoschi
*   Adapted for annotation tasks
*/
class DynamicOverlap3Plus1Plus1 implements EventListener {
    private Counters perSuite;
    private boolean moreLogging

    @Override
    void onEvent(final List<Assignment> events, final ComradeClient client) {
        long start = System.currentTimeMillis();
        loadGlobals(client.storage)

        events.groupBy { it.poolId }.each {poolId, eventsByPool ->
            HpProcessor hpProcessor = new HpProcessor(client.storage, poolId)
            client.logger.info(hpProcessor.toString())

            def newAnswers = ComradeUtils.assignmentToAnswers(eventsByPool)
                    .findAll({ it.getStatus() == AssignmentStatus.SUBMITTED })  // Annotations specific
            client.logger.info("New answers size={}", newAnswers.size())
            hpProcessor.store(newAnswers)
            client.logger.info("New answers stored")

            def groupedNewAnswers = newAnswers.findAll({ !it.task.isHoneypot() && !it.task.isHint() }).groupBy { it.task }

            groupedNewAnswers.each { task, answers ->
                List<Answer> allAnswersForTask = ComradeUtils.saveNewAnswersAndLoadAll(answers, task, client.storage)

                if (allAnswersForTask.size() >= 3 && allAnswersForTask.size() < 5) {
                    // Annotations specific:
                    def answersGroupedBySolutions = allAnswersForTask.groupBy(
                            { it.solution.outputValues.findAll({ it.key != 'result_unformat' }) }
                    )
                    def solutionWinner = answersGroupedBySolutions.find(
                            { key, value -> value.size() >= 3 }
                    )
                    // End of "annotations specific"
                    if (solutionWinner == null) {
                        if (moreLogging) {
                            client.logger.info("No solutions for input {} with enough votes: {}", task.inputValues, allAnswersForTask.collect {it.solution.outputValues})
                        }
                        hpProcessor.add(task)
                    } else {
                        if (moreLogging) {
                            client.logger.info("Solution winner for input {}: {}", task.inputValues, solutionWinner.key)
                        }
                    }
                } else {
                    if (moreLogging) {
                        client.logger.info("There are {} solution(s) for input {}", allAnswersForTask.size(), task.inputValues)
                    }
                }
            }
            ComradeUtils.uploadIfNecessary(perSuite, hpProcessor, client, poolId, client.storage)
        }

        client.logger.info("Time: {}", System.currentTimeMillis() - start)
    }

    private void loadGlobals(Storage storage) {

        def params = storage.get("params").orElseThrow({
            new RuntimeException("Init storage should contain params")
        })

        perSuite = new Counters(params.normalPerSuite, params.honeysPerSuite, params.hintsPerSuite)
        moreLogging = params.moreLogging != null && params.moreLogging
    }
}

package ru.yandex.qe.hitman.comrade.script.execution.groovy.example

import ru.yandex.qe.hitman.comrade.script.model.ComradeClient
import ru.yandex.qe.hitman.comrade.script.model.EventListener
import ru.yandex.qe.hitman.comrade.script.model.Storage
import ru.yandex.qe.hitman.comrade.script.model.operation.IncrementTaskOverlapOperation
import ru.yandex.qe.hitman.comrade.script.model.toloka.Assignment
import ru.yandex.qe.hitman.comrade.script.model.toloka.AssignmentStatus
import ru.yandex.qe.hitman.comrade.script.model.toloka.IncrementOverlapParams
import ru.yandex.qe.hitman.comrade.script.util.Answer
import ru.yandex.qe.hitman.comrade.script.util.ComradeUtils


class DynamicOverlap implements EventListener {
    private boolean moreLogging

    @Override
    void onEvent(List<Assignment> events, ComradeClient client) {
        long start = System.currentTimeMillis();
        loadGlobals(client.storage);

        int minOverlap = MIN_OVERLAP;
        int maxOverlap = MAX_OVERLAP;
        boolean isSpotterMetricsOnly = IS_SPOTTER_METRICS_ONLY;

        events.groupBy { it.poolId }.each {poolId, eventsByPool ->
            def newAnswers = ComradeUtils.assignmentToAnswers(eventsByPool)
                    .findAll({ it.getStatus() == AssignmentStatus.SUBMITTED || it.getStatus() == AssignmentStatus.ACCEPTED })
            client.logger.info("New answers size={}", newAnswers.size())

            def groupedNewAnswers = newAnswers.findAll({ !it.task.isHoneypot() && !it.task.isHint() }).groupBy { it.task }
            groupedNewAnswers.each { task, answers ->
                List<Answer> allAnswersForTask = ComradeUtils.saveNewAnswersAndLoadAll(answers, task, client.storage)

                if (allAnswersForTask.size() >= minOverlap && allAnswersForTask.size() < maxOverlap) {
                    def answersGroupedBySolutions = allAnswersForTask.groupBy { it.solution.outputValues.findAll({ it.key == 'query' || it.key == 'annotation' || it.key == 'artificial' || it.key == 'bad'  }) }
                    if (isSpotterMetricsOnly) {
                        answersGroupedBySolutions = allAnswersForTask.groupBy { countKeyWords(it.solution.outputValues['annotation'] ? it.solution.outputValues['annotation'] : "") }
                    }
                    def solutionWinner = answersGroupedBySolutions.find { it.value.size() >= minOverlap }
                    if (solutionWinner == null) {
                        if (moreLogging) {
                            client.logger.info("No solutions for input {} with {} votes or more: {}", task.inputValues, minOverlap.toString(), allAnswersForTask.collect {it.solution.outputValues})
                        }
                        client.enqueueOperation(new IncrementTaskOverlapOperation(
                                new IncrementOverlapParams(task.getTaskId(), maxOverlap)))
                    } else {
                        if (moreLogging) {
                            client.logger.info("Solution winner for input {}: {}", task.inputValues, solutionWinner.key)
                        }
                    }
                } else {
                    if (moreLogging) {
                        client.logger.info("There are {} solution(s) for input {}", allAnswersForTask.size(), task.inputValues)
                    }

                    client.enqueueOperation(new IncrementTaskOverlapOperation(
                            new IncrementOverlapParams(task.getTaskId(), minOverlap)))
                }
            }
        }

        client.logger.info("Time: {}", System.currentTimeMillis() - start)
    }

    private void loadGlobals(Storage storage) {

        def params = storage.get("params").orElseThrow({
            new RuntimeException("Init storage should contain params")
        })

        moreLogging = params.moreLogging != null && params.moreLogging
    }

     ArrayList countKeyWords(text) {
        def res = []
        def key_words = ["алиса", "алис", "яндекс", "дальше", "назад", "ниже", "выше", "отмена", "поехали", "yandex", "alisa", "alice"]
        key_words.each{
            key_word -> res.add(text.toLowerCase().split(' ').findAll{it.equals(key_word)}.size())
        }
        return res
    }
}

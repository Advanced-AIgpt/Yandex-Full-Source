package ru.yandex.alice.paskill.dialogovo.processor;

import java.util.concurrent.Future;

import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessRequest;
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessResult;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;

public interface SkillRequestProcessor {
    /**
     * Do request to skill's webhook and process result
     *
     * @param req request data
     * @return result
     */
    SkillProcessResult process(Context ctx, SkillProcessRequest req);

    Future<SkillProcessResult> processAsync(Context context, SkillProcessRequest request);
}

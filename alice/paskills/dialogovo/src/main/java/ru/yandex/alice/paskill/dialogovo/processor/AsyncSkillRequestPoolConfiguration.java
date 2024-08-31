package ru.yandex.alice.paskill.dialogovo.processor;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.alice.paskill.dialogovo.utils.executor.DialogovoInstrumentedExecutorService;
import ru.yandex.alice.paskill.dialogovo.utils.executor.ExecutorsFactory;

@Configuration
public class AsyncSkillRequestPoolConfiguration {

    @Bean(value = "asyncSkillRequestServiceExecutor", destroyMethod = "shutdownNow")
    public DialogovoInstrumentedExecutorService asyncSkillRequestServiceExecutor(ExecutorsFactory executorsFactory) {
        return executorsFactory.cachedBoundedThreadPool(2, 50, 50, "async-skill-service");
    }
}

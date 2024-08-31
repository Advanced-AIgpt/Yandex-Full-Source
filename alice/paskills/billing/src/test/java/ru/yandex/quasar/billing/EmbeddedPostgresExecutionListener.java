package ru.yandex.quasar.billing;

import org.junit.platform.launcher.TestExecutionListener;
import org.junit.platform.launcher.TestPlan;

public class EmbeddedPostgresExecutionListener implements TestExecutionListener {

    static volatile PostgresTestingConfiguration.PostgresInstance instance;

    @Override
    public void testPlanExecutionFinished(TestPlan testPlan) {
        if (instance != null) {
            System.out.println("STOPPING EMBEDDED DB");
            try {
                instance.cleanupInstance();
            } catch (Exception e) {
                throw new RuntimeException(e);
            }
            instance = null;
        }
    }

}

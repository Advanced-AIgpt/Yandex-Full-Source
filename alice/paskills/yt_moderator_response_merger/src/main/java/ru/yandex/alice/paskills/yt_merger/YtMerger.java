package ru.yandex.alice.paskills.yt_merger;

import com.beust.jcommander.JCommander;
import org.apache.logging.log4j.Level;
import org.apache.logging.log4j.core.config.Configurator;
import org.apache.logging.log4j.core.config.DefaultConfiguration;

public class YtMerger {

    private YtMerger() {
        throw new UnsupportedOperationException();
    }

    public static void main(String[] args) {
        Configurator.initialize(new DefaultConfiguration());
        Configurator.setRootLevel(Level.INFO);
        CommandLineArgs commandLineArgs = new CommandLineArgs();
        JCommander jCommander = JCommander.newBuilder()
                .addObject(commandLineArgs)
                .build();
        jCommander.parse(args);
        if (commandLineArgs.isHelp()) {
            jCommander.usage();
        } else {
            String ytToken = System.getenv("YT_TOKEN");
            if (ytToken == null) {
                throw new IllegalArgumentException("Missing YT client credentials. " +
                        "Please set YT_TOKEN environment variable");
            }
            YtMergerImpl ytMerger = new YtMergerImpl(commandLineArgs, ytToken);
            ytMerger.run();
        }
    }

}

package ru.yandex.alice.paskills.my_alice.blocks.recommender;

import java.util.List;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

@Data
public class RecommenderResponse {

    private static final Logger logger = LogManager.getLogger();

    private final List<RecommenderItem> items;
    @JsonProperty("recommendation_type")
    private final String recommendationType;
    @JsonProperty("recommendation_source")
    private final String recommendationSource;


}

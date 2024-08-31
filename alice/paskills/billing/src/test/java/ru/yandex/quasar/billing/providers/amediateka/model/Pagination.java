package ru.yandex.quasar.billing.providers.amediateka.model;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Getter;

@Getter
public class Pagination {
    private Integer limit;
    private Integer offset;
    private Integer count;

    @JsonProperty("total_count")
    private Integer totalCount;

    /**
     * @return if this page is last and no other pages should be requested
     */
    @JsonIgnore
    public boolean isLastPage() {
        return offset + count == totalCount;
    }

    /**
     * @return `offset` to be used to get next page
     */
    @JsonIgnore
    public Integer nextOffset() {
        return offset + count;
    }

}

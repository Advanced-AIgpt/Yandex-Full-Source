package ru.yandex.alice.paskills.my_alice.bunker.main_promo;

import java.io.IOException;
import java.util.List;

import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.json.JsonTest;
import org.springframework.boot.test.json.GsonTester;

@JsonTest
class DataTest {

    @Autowired
    private GsonTester<Data> gsonTester;

    @Test
    void emptyData() throws IOException {
        Data expected = new Data(1.0, List.of());
        gsonTester.parse("{}")
                .assertThat().isEqualTo(expected);
    }

    @Test
    void dataValues() throws IOException {
        Data expected = new Data(.123, List.of());

        gsonTester.parse("{\"show_probability\":0.123,\"items\":[null]}")
                .assertThat().isEqualTo(expected);
    }

    @Test
    void showProbabilityLimits() throws IOException {
        Data expectedMin = new Data(0.0, List.of());
        Data expectedMax = new Data(1.0, List.of());

        gsonTester.parse("{\"show_probability\":-1,\"items\":[null]}")
                .assertThat().isEqualTo(expectedMin);
        gsonTester.parse("{\"show_probability\":0,\"items\":[null]}")
                .assertThat().isEqualTo(expectedMin);
        gsonTester.parse("{\"show_probability\":0.0,\"items\":[null]}")
                .assertThat().isEqualTo(expectedMin);
        gsonTester.parse("{\"show_probability\":1,\"items\":[null]}")
                .assertThat().isEqualTo(expectedMax);
        gsonTester.parse("{\"show_probability\":1.0,\"items\":[null]}")
                .assertThat().isEqualTo(expectedMax);
        gsonTester.parse("{\"show_probability\":10.0,\"items\":[null]}")
                .assertThat().isEqualTo(expectedMax);
    }

    @Test
    void emptyBlock() throws IOException {
        Data expected = new Data(1.0, List.of(
                new Data.Block(null, 1.0, "everybody", List.of())
        ));

        gsonTester.parse("{\"items\":[{}]}")
                .assertThat().isEqualTo(expected);
    }

    @Test
    void blockData() throws IOException {
        Data expected = new Data(1.0, List.of(
                new Data.Block("ID", 10.0, "required", List.of())
        ));

        gsonTester.parse("{\"items\":[{\"id\":\"ID\",\"weight\":10,\"auth\":\"required\",\"items\":[null]}]}")
                .assertThat().isEqualTo(expected);
    }

    @Test
    void blockWeightLimits() throws IOException {
        Data expected = new Data(1.0, List.of(
                new Data.Block(null, 0.0, "everybody", List.of())
        ));

        gsonTester.parse("{\"items\":[{\"weight\":-10}]}")
                .assertThat().isEqualTo(expected);
    }

    @Test
    void emptyCard() throws IOException {
        Data expected = new Data(1.0, List.of(
                new Data.Block(null, 1.0, "everybody", List.of(
                        new Data.Card(null, 1.0, null, null, null, null, null)
                ))
        ));

        gsonTester.parse("{\"items\":[{\"items\":[{}]}]}")
                .assertThat().isEqualTo(expected);
    }

    @Test
    void cardData() throws IOException {
        Data expected = new Data(1.0, List.of(
                new Data.Block(null, 1.0, "everybody", List.of(
                        new Data.Card("ID", 10.0, "CAPTION", "TEXT", "BUTTON", "IMG", "#fc0")
                ))
        ));

        gsonTester.parse("{\"items\":[{\"items\":[{\"id\":\"ID\",\"weight\":10,\"caption\":\"CAPTION\"," +
                "\"text\":\"TEXT\",\"button\":\"BUTTON\",\"image\":\"IMG\",\"color\":\"#fc0\"}]}]}")
                .assertThat().isEqualTo(expected);
    }


    @Test
    void cardWeightLimits() throws IOException {
        Data expected = new Data(1.0, List.of(
                new Data.Block(null, 1.0, "everybody", List.of(
                        new Data.Card(null, 0.0, null, null, null, null, null)
                ))
        ));

        gsonTester.parse("{\"items\":[{\"items\":[{\"weight\":-1}]}]}")
                .assertThat().isEqualTo(expected);
    }
}

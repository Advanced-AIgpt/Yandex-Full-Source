package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.providers;

import java.io.IOException;
import java.io.InputStream;
import java.util.List;
import java.util.regex.Pattern;
import java.util.stream.Stream;

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import com.fasterxml.jackson.core.type.TypeReference;
import com.fasterxml.jackson.databind.ObjectMapper;
import lombok.AllArgsConstructor;
import lombok.Data;
import lombok.NoArgsConstructor;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.TestInstance;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.Arguments;
import org.junit.jupiter.params.provider.MethodSource;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.context.SpringBootTest;

import ru.yandex.alice.kronstadt.core.layout.TextWithTts;
import ru.yandex.alice.paskill.dialogovo.config.TestConfigProvider;
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.entities.CountableIngredient;
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.entities.Ingredient;
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.entities.UncountableIngredient;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertTrue;

@SpringBootTest(classes = {TestConfigProvider.class}, webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT)
@TestInstance(TestInstance.Lifecycle.PER_CLASS)
class IngredientProviderTest {

    private static final Pattern VALID_INGREDIENT_NAME_TEXT =
            Pattern.compile("^[а-яА-ЯёЁ -]+$", Pattern.UNICODE_CHARACTER_CLASS);
    private static final Pattern VALID_INGREDIENT_NAME_TTS =
            Pattern.compile("^[а-яА-ЯёЁ +-]+$", Pattern.UNICODE_CHARACTER_CLASS);

    @Autowired
    private IngredientProvider ingredientProvider;

    public static Stream<Arguments> getIngredients() throws IOException {
        ObjectMapper objectMapper = new ObjectMapper();
        try (InputStream inputStream = IngredientProviderTest.class.getClassLoader()
                .getResourceAsStream("recipes/ingredients.json")) {
            return objectMapper.readValue(inputStream, new TypeReference<List<TestId>>() {
            }).stream()
                    .map(TestId::getId)
                    .map(Arguments::of);
        }
    }

    @Test
    public void testIngridientListNotEmpty() {
        assertTrue(ingredientProvider.size() > 0);
    }

    @ParameterizedTest(name = "validate child of ingredient {0}")
    @MethodSource("getIngredients")
    public void testAllChildIdsAreValid(String ingredientName) throws JsonEntityProvider.EntityNotFound {
        Ingredient ingredient = ingredientProvider.get(ingredientName);
        for (String childId : ingredient.getChildren()) {
            // test that JsonEntityProvider.EntityNotFound is not thrown
            ingredientProvider.get(childId);
        }
    }

    @ParameterizedTest(name = "test ingredient {0}")
    @MethodSource("getIngredients")
    public void testIngredientNames(String ingredientName) throws JsonEntityProvider.EntityNotFound {
        Ingredient ingredient = ingredientProvider.get(ingredientName);
        assertNotNull(ingredient.getId());
        validateIngredientName(ingredient.getId(), ingredient.getName());
        if (ingredient instanceof CountableIngredient) {
            var countableIngredient = (CountableIngredient) ingredient;
            for (var name : countableIngredient.getPluralForms()) {
                validateIngredientName(ingredient.getId(), name);
            }
        } else if (ingredient instanceof UncountableIngredient) {
            var uncountableIngredient = (UncountableIngredient) ingredient;
            validateIngredientName(ingredient.getId(), uncountableIngredient.getInflectedName());
        }
    }

    public void validateIngredientName(String id, TextWithTts name) {
        if (!"none".equals(id)) {
            assertTrue(VALID_INGREDIENT_NAME_TEXT.matcher(name.getText()).matches());
            assertTrue(VALID_INGREDIENT_NAME_TTS.matcher(name.getTts()).matches());
        } else {
            assertEquals(name, TextWithTts.EMPTY);
        }
    }

    @Data
    @AllArgsConstructor
    @NoArgsConstructor
    @JsonIgnoreProperties(ignoreUnknown = true)
    static class TestId {
        private String id;
    }
}

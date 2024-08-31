package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.providers;

import java.io.IOException;
import java.io.InputStream;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.regex.Pattern;
import java.util.stream.Collectors;
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

import ru.yandex.alice.paskill.dialogovo.config.TestConfigProvider;
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.IngredientWithQuantity;
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.Recipe;
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.RecipeStep;
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.TimerDependency;
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.entities.CountableIngredient;
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.entities.Ingredient;
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.entities.UncountableIngredient;
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.measurement.ExactQuantity;
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.measurement.MeasurementUnit;
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.measurement.Quantity;
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.measurement.ToTaste;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertTrue;

@SpringBootTest(classes = TestConfigProvider.class, webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT)
@TestInstance(TestInstance.Lifecycle.PER_CLASS)
class RecipeProviderConfigurationTest {

    private static final Pattern ID_PATTERN = Pattern.compile("[a-zA-Z][a-zA-Z0-9_-]+");
    @Autowired
    private RecipeProvider recipeProvider;

    public static Stream<Arguments> getRecipes() throws IOException {
        ObjectMapper objectMapper = new ObjectMapper();
        try (InputStream inputStream = IngredientProviderTest.class.getClassLoader()
                .getResourceAsStream("recipes/recipes.json")) {
            return objectMapper.readValue(inputStream, new TypeReference<List<TestId>>() {
            }).stream()
                    .map(TestId::getId)
                    .map(Arguments::of);
        }
    }

    @Test
    void hasRecipes() {
        assertTrue(recipeProvider.size() > 0);
    }

    @Test
    void allRecipeIdsAreUnique() {
        Set<String> ids = new HashSet<>();
        for (Recipe recipe : recipeProvider.getAllRecipes()) {
            assertFalse(ids.contains(recipe.getId()), String.format("Found duplicate recipe id: %s", recipe.getId()));
            ids.add(recipe.getId());
        }
    }

    @ParameterizedTest(name = "validate recipe {0}")
    @MethodSource("getRecipes")
    void assertRecipeIsValid(String recipeName) {
        Recipe recipe = recipeProvider.get(recipeName).orElseThrow();
        assertNotNull(recipe.getId(), "Recipe id not set");
        assertTrue(ID_PATTERN.matcher(recipe.getId()).matches(),
                String.format("Recipe id %s contains invalid characters", recipe.getId()));
        assertNotNull(recipe.getName(), "Recipe name not set");
        assertNotNull(recipe.getInflectedNameCases(), "Inflected name not set");
        assertNotNull(recipe.getCookingTimeText(), "Cooking time not specified");
        assertTrue(recipe.getNumberOfServings() > 0, "Number of servings should be greater than 0");
        assertTrue(recipe.getIngredients().size() > 0, "Recipe has no ingredients");
        assertTrue(recipe.getSteps().size() > 0, "Recipe has no steps");

        for (IngredientWithQuantity ingredientWithQuantity : recipe.getIngredients()) {
            Quantity quantity = ingredientWithQuantity.getQuantity();
            if (quantity instanceof ToTaste) {
                assertTrue(ingredientWithQuantity.getIngredient() instanceof UncountableIngredient,
                        "[" + ingredientWithQuantity.getIngredient().getName().getText() +
                                "]ToTaste amount can be used only with uncountable ingredients");
            } else if (quantity instanceof ExactQuantity) {
                MeasurementUnit measurementUnit = ((ExactQuantity) quantity).getMeasurementUnit();
                Ingredient ingredient = ingredientWithQuantity.getIngredient();
                if (ingredient instanceof CountableIngredient) {
                    assertEquals(measurementUnit, MeasurementUnit.NONE,
                            "Countable ingredients can be used only with MeasurementUnit.NONE"
                    );
                }
                if (ingredient instanceof UncountableIngredient) {
                    assertNotEquals(
                            measurementUnit,
                            MeasurementUnit.NONE,
                            String.format(
                                    "Uncountable ingredient %s cannot be used with MeasurementUnit.NONE",
                                    ingredient.getId())
                    );
                }

            }
        }

        Set<String> timers = new HashSet<>();
        Counter timerDependencyPerTimer = new Counter();
        for (int stepId = 0; stepId < recipe.getSteps().size(); stepId++) {
            RecipeStep step = recipe.getSteps().get(stepId);

            if (step.hasTimer()) {
                var timer = step.getTimer();
                assertNotNull(timer.getDuration());
                assertNotNull(timer.getText());
                timers.add(step.getTimer().id(stepId));
            }

            List<TimerDependency> timerDependencies = step.getDependencies().stream()
                    .filter(d -> d instanceof TimerDependency)
                    .map(d -> (TimerDependency) d)
                    .collect(Collectors.toList());
            for (var timerDependency : timerDependencies) {
                assertTrue(timerDependency.getStepId() < stepId,
                        String.format(
                                "Timer dependency cannot depend on future steps: step %d depends on step %d",
                                stepId,
                                timerDependency.getStepId()
                        ));
                RecipeStep timerCreatedAt = recipe.getSteps().get(timerDependency.getStepId());
                assertTrue(timerCreatedAt.hasTimer(),
                        String.format("Step %d depends on timer from step %d, but latter step doesn't set any timers",
                                stepId, timerDependency.getStepId()));
                timerDependencyPerTimer.inc(timerCreatedAt.getTimer().id(timerDependency.getStepId()));
            }
        }

        assertEquals(timers, timerDependencyPerTimer.keySet(), recipe.getId());
        for (var timerAndCount : timerDependencyPerTimer.entrySet()) {
            assertEquals(timerAndCount.getValue(), 1,
                    String.format("Timer %s is referenced more than 1 time", timerAndCount.getKey()));
        }
    }

    @Test
    void printRecipes() {
        Collection<Recipe> recipes = recipeProvider.getAllRecipes();

        recipes.stream()
                .map(Recipe::print)
                .forEach(System.out::println);
    }

    private static class Counter extends HashMap<String, Integer> {

        public void inc(String key) {
            Integer value = get(key);
            if (value == null) {
                put(key, 1);
            } else {
                put(key, value + 1);
            }
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

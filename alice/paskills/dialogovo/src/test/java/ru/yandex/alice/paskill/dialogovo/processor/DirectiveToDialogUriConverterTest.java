package ru.yandex.alice.paskill.dialogovo.processor;

import java.util.List;

import com.fasterxml.jackson.core.JsonProcessingException;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.context.SpringBootTest;

import ru.yandex.alice.kronstadt.core.RequestContext;
import ru.yandex.alice.paskill.dialogovo.config.TestConfigProvider;
import ru.yandex.alice.paskill.dialogovo.midleware.DialogovoRequestContext;
import ru.yandex.alice.paskill.dialogovo.scenarios.Scenarios;
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.processors.GetIngredientListCallbackDirective;

import static java.util.Collections.emptyList;
import static org.junit.jupiter.api.Assertions.assertEquals;

@SpringBootTest(classes = {TestConfigProvider.class})
class DirectiveToDialogUriConverterTest {

    @Autowired
    private DirectiveToDialogUriConverter converter;
    @Autowired
    private RequestContext context;
    @Autowired
    private DialogovoRequestContext dialogovoContext;

    @BeforeEach
    void setUp() {
        dialogovoContext.setScenario(Scenarios.RECIPES);
    }

    @Test
    public void testEmptyDirectiveList() throws JsonProcessingException {
        assertEquals(
                "dialog://?",
                converter.convertDirectives(emptyList()).toASCIIString()
        );
    }

    @Test
    public void testWithRecipeIngredientListDirective() throws JsonProcessingException {
        GetIngredientListCallbackDirective directive = new GetIngredientListCallbackDirective("sirniki");
        assertEquals(
                "dialog://?directives=[%7B%22name%22:%22recipes_get_ingredient_list%22,%22ignore_answer%22:false," +
                        "%22payload%22:%7B%22recipe_id%22:%22sirniki%22," +
                        "%22@scenario_name%22:%22ExternalSkillRecipes%22%7D,%22type%22:%22server_action%22%7D]",
                converter.convertDirectives(List.of(directive)).toString()
        );
    }

}

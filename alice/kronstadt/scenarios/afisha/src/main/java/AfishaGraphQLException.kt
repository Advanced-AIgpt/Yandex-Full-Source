package ru.yandex.alice.kronstadt.scenarios.afisha

import ru.yandex.alice.kronstadt.scenarios.afisha.model.response.Error

class AfishaGraphQLException(errors: List<Error>) : Exception("Request to afisha graphQL failed with errors: $errors")

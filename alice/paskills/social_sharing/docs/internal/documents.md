# API создания и коммита документов

Пример графа: social_sharing_test_skill

## Создание кандидатов на run-стадии

Чтобы создать ссылку на run-стадии и вставить её в ответ сценария,
нужно отправить запрос в виде `TCreateCandidateRequest` в аппхост-бекенд
ALICE_SOCIAL_SHARE с типом `create_candidate_request`.

В ответ вершина выдаст `TCreateCandidateResponse` в типе `create_candidate_response`

Кандидат обязательно нужно закоммитить в commit-стадии, иначе ссылка не будет доступна пользователям.

## Коммит кандидатов

Нужно отправить запрос `TCommitCandidateRequest` с типом `commit_candidate_request`.
Вершина возвращает `TCommitCandidateResponse` в типе `commit_candidate`

После успешного коммита ссылка будет доступна внешним пользователям.

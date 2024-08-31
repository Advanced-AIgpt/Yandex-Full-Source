insert into "draftUserAgreements" (
    id
    , "skillId"
    , "order"
    , name
    , url
    , "createdAt"
    , "updatedAt"
) VALUES (
    ?
    , ?
    , ?
    , ?
    , ?
    , now()
    , now()
);

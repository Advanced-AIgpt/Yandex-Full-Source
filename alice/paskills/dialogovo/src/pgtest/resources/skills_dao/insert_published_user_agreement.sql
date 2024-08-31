insert into "publishedUserAgreements" (
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

insert into "publishedIntents" (id,
                                "skillId",
                                "formName",
                                "humanReadableName",
                                "isActivation",
                                "createdAt",
                                "updatedAt")
values (?,
        ?,
        ?,
        ?,
        ?::boolean,
        now(),
        now());

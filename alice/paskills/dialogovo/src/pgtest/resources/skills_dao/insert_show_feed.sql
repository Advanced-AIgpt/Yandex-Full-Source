insert into "showFeeds" (id,
                         "skillId",
                         name,
                         "nameTts",
                         description,
                         "onAir",
                         type)
values (?,
        ?,
        ?,
        null,
        ?,
        true,
        'morning'::"enum_show_types");

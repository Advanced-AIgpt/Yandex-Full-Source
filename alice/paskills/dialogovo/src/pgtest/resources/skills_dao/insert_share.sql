insert into skill_user_share(id,
                             "createdAt",
                             "updatedAt",
                             skill_id,
                             user_id)
values (?,
        now(),
        now(),
        ?,
        ?);

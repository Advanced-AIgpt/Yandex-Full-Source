insert into users (id,
                   name,
                   "createdAt",
                   "updatedAt",
                  "featureFlags"
)
values (?,
        ?,
        now(),
        now(),
        '{"abc": 123}'::jsonb);

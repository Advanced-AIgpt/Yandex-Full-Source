insert into images (id,
                    "skillId",
                    "url",
                    "type",
                    "size",
                    "createdAt",
                    "updatedAt")
values (?,
        ?,
        ?,
        ?::enum_images_type,
        10,
        now(),
        now());

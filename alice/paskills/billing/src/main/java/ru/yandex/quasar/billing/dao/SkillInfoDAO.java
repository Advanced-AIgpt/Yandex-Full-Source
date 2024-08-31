package ru.yandex.quasar.billing.dao;

import java.util.Optional;

import org.springframework.data.jdbc.repository.query.Query;
import org.springframework.data.repository.CrudRepository;
import org.springframework.data.repository.query.Param;

import ru.yandex.alice.library.routingdatasource.ReadOnlyTransactional;


public interface SkillInfoDAO extends CrudRepository<SkillInfo, Long> {
    @ReadOnlyTransactional
    @Query("select * from skill_info where skill_uuid = :skillUuid")
    Optional<SkillInfo> findBySkillId(@Param("skillUuid") String skillId);

    @ReadOnlyTransactional
    @Query("select * from skill_info where skill_info_id = :skillInfoId")
    Optional<SkillInfo> findBySkillId(@Param("skillInfoId") Long skillInfoId);
}

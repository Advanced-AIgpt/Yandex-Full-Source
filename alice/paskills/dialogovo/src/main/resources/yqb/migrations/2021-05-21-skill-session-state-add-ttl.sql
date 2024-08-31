ALTER TABLE `skill_user_state` SET (TTL = Interval("P1825D") ON changed_at);
ALTER TABLE `skill_session_state` SET (TTL = Interval("P1W") ON changed_at);

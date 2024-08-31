--
-- PostgreSQL database dump
--

-- Dumped from database version 10.15 (Ubuntu 10.15-201)
-- Dumped by pg_dump version 10.14

SET statement_timeout = 0;
SET lock_timeout = 0;
SET idle_in_transaction_session_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
--SELECT pg_catalog.set_config('search_path', 'public', false);
SET check_function_bodies = false;
SET xmloption = content;
SET client_min_messages = warning;
SET row_security = off;

/*--
-- Name: pg_repack; Type: EXTENSION; Schema: -; Owner: -
--

CREATE EXTENSION IF NOT EXISTS pg_repack WITH SCHEMA public;


--
-- Name: EXTENSION pg_repack; Type: COMMENT; Schema: -; Owner: -
--

COMMENT ON EXTENSION pg_repack IS 'Reorganize tables in PostgreSQL databases with minimal locks';


--
-- Name: plpgsql; Type: EXTENSION; Schema: -; Owner: -
--

CREATE EXTENSION IF NOT EXISTS plpgsql WITH SCHEMA pg_catalog;


--
-- Name: EXTENSION plpgsql; Type: COMMENT; Schema: -; Owner: -
--

COMMENT ON EXTENSION plpgsql IS 'PL/pgSQL procedural language';


--
-- Name: amcheck; Type: EXTENSION; Schema: -; Owner: -
--

CREATE EXTENSION IF NOT EXISTS amcheck WITH SCHEMA public;


--
-- Name: EXTENSION amcheck; Type: COMMENT; Schema: -; Owner: -
--

COMMENT ON EXTENSION amcheck IS 'functions for verifying relation integrity';


--
-- Name: heapcheck; Type: EXTENSION; Schema: -; Owner: -
--

CREATE EXTENSION IF NOT EXISTS heapcheck WITH SCHEMA public;


--
-- Name: EXTENSION heapcheck; Type: COMMENT; Schema: -; Owner: -
--

COMMENT ON EXTENSION heapcheck IS 'functions for verifying relation integrity';


--
-- Name: pg_stat_statements; Type: EXTENSION; Schema: -; Owner: -
--

CREATE EXTENSION IF NOT EXISTS pg_stat_statements WITH SCHEMA public;


--
-- Name: EXTENSION pg_stat_statements; Type: COMMENT; Schema: -; Owner: -
--

COMMENT ON EXTENSION pg_stat_statements IS 'track execution statistics of all SQL statements executed';


--
-- Name: pg_stat_kcache; Type: EXTENSION; Schema: -; Owner: -
--

CREATE EXTENSION IF NOT EXISTS pg_stat_kcache WITH SCHEMA public;


--
-- Name: EXTENSION pg_stat_kcache; Type: COMMENT; Schema: -; Owner: -
--

COMMENT ON EXTENSION pg_stat_kcache IS 'Kernel statistics gathering';


--
-- Name: pg_trgm; Type: EXTENSION; Schema: -; Owner: -
--

CREATE EXTENSION IF NOT EXISTS pg_trgm WITH SCHEMA public;


--
-- Name: EXTENSION pg_trgm; Type: COMMENT; Schema: -; Owner: -
--

COMMENT ON EXTENSION pg_trgm IS 'text similarity measurement and index searching based on trigrams';


--
-- Name: postgres_fdw; Type: EXTENSION; Schema: -; Owner: -
--

CREATE EXTENSION IF NOT EXISTS postgres_fdw WITH SCHEMA public;


--
-- Name: EXTENSION postgres_fdw; Type: COMMENT; Schema: -; Owner: -
--

COMMENT ON EXTENSION postgres_fdw IS 'foreign-data wrapper for remote PostgreSQL servers';*/


--
-- Name: uuid-ossp; Type: EXTENSION; Schema: -; Owner: -
--

CREATE EXTENSION IF NOT EXISTS "uuid-ossp" WITH SCHEMA public;


--
-- Name: EXTENSION "uuid-ossp"; Type: COMMENT; Schema: -; Owner: -
--

COMMENT ON EXTENSION "uuid-ossp" IS 'generate universally unique identifiers (UUIDs)';


--
-- Name: enum_devicesTestingRecords_type; Type: TYPE; Schema: public; Owner: -
--

CREATE TYPE public."enum_devicesTestingRecords_type" AS ENUM (
    'online',
    'offline'
);


--
-- Name: enum_drafts_activationPhrasesCommonness; Type: TYPE; Schema: public; Owner: -
--

CREATE TYPE public."enum_drafts_activationPhrasesCommonness" AS ENUM (
    'green',
    'red'
);


--
-- Name: enum_drafts_channel; Type: TYPE; Schema: public; Owner: -
--

CREATE TYPE public.enum_drafts_channel AS ENUM (
    'aliceSkill',
    'organizationChat',
    'smartHome',
    'thereminvox',
    'aliceNewsSkill'
);


--
-- Name: enum_drafts_exactSurfaces; Type: TYPE; Schema: public; Owner: -
--

CREATE TYPE public."enum_drafts_exactSurfaces" AS ENUM (
    'auto',
    'desktop',
    'mobile',
    'navigator',
    'station',
    'watch'
);


--
-- Name: enum_drafts_requiredInterfaces; Type: TYPE; Schema: public; Owner: -
--

CREATE TYPE public."enum_drafts_requiredInterfaces" AS ENUM (
    'browser',
    'screen'
);


--
-- Name: enum_drafts_skillAccess; Type: TYPE; Schema: public; Owner: -
--

CREATE TYPE public."enum_drafts_skillAccess" AS ENUM (
    'public',
    'hidden',
    'private'
);


--
-- Name: enum_drafts_status; Type: TYPE; Schema: public; Owner: -
--

CREATE TYPE public.enum_drafts_status AS ENUM (
    'inDevelopment',
    'reviewRequested',
    'reviewCancelled',
    'reviewApproved',
    'reviewRejected',
    'deployRequested',
    'deployCompleted',
    'deployRejected',
    'testRequired',
    'meetingApproval',
    'waitingForTesting',
    'devicesTesting',
    'devicesTestingCompleted'
);


--
-- Name: enum_drafts_surfaceBlacklist; Type: TYPE; Schema: public; Owner: -
--

CREATE TYPE public."enum_drafts_surfaceBlacklist" AS ENUM (
    'auto',
    'desktop',
    'mobile',
    'navigator',
    'station',
    'watch'
);


--
-- Name: enum_drafts_surfaceWhitelist; Type: TYPE; Schema: public; Owner: -
--

CREATE TYPE public."enum_drafts_surfaceWhitelist" AS ENUM (
    'auto',
    'desktop',
    'mobile',
    'navigator',
    'station',
    'watch'
);


--
-- Name: enum_emailAlerts_type; Type: TYPE; Schema: public; Owner: -
--

CREATE TYPE public."enum_emailAlerts_type" AS ENUM (
    'pingUnanswers'
);


--
-- Name: enum_ferrymanJobs_status; Type: TYPE; Schema: public; Owner: -
--

CREATE TYPE public."enum_ferrymanJobs_status" AS ENUM (
    'created',
    'uploadedToYT',
    'submittedToFerryman',
    'indexing',
    'done',
    'error'
);


--
-- Name: enum_ferrymanJobs_type; Type: TYPE; Schema: public; Owner: -
--

CREATE TYPE public."enum_ferrymanJobs_type" AS ENUM (
    'kvSaaS',
    'SaaS'
);


--
-- Name: enum_images_type; Type: TYPE; Schema: public; Owner: -
--

CREATE TYPE public.enum_images_type AS ENUM (
    'skillSettings',
    'skillCard'
);


--
-- Name: enum_newsFeeds_type; Type: TYPE; Schema: public; Owner: -
--

CREATE TYPE public."enum_newsFeeds_type" AS ENUM (
    'rss',
    'ftp',
    'yt'
);


--
-- Name: enum_operations_type; Type: TYPE; Schema: public; Owner: -
--

CREATE TYPE public.enum_operations_type AS ENUM (
    'skillCreated',
    'skillWithdrawn',
    'skillStopped',
    'skillDeleted',
    'reviewRequested',
    'reviewCancelled',
    'reviewApproved',
    'reviewRejected',
    'deployRequested',
    'deployCompleted',
    'deployRejected',
    'settingsReviewApproved',
    'deviceTestPrepared',
    'deviceTestStarted',
    'deviceTestCompleted',
    'skillNominated',
    'skillNominationRotationWithdrawn',
    'skillNominationRotationCompleted'
);


--
-- Name: enum_skills_activationPhrasesCommonness; Type: TYPE; Schema: public; Owner: -
--

CREATE TYPE public."enum_skills_activationPhrasesCommonness" AS ENUM (
    'green',
    'red'
);


--
-- Name: enum_skills_alicePrizeNomination; Type: TYPE; Schema: public; Owner: -
--

CREATE TYPE public."enum_skills_alicePrizeNomination" AS ENUM (
    'education_reference',
    'games_trivia_accessories',
    'kids',
    'useful',
    'special'
);


--
-- Name: enum_skills_channel; Type: TYPE; Schema: public; Owner: -
--

CREATE TYPE public.enum_skills_channel AS ENUM (
    'aliceSkill',
    'organizationChat',
    'smartHome',
    'thereminvox',
    'aliceNewsSkill'
);


--
-- Name: enum_skills_exactSurfaces; Type: TYPE; Schema: public; Owner: -
--

CREATE TYPE public."enum_skills_exactSurfaces" AS ENUM (
    'auto',
    'desktop',
    'mobile',
    'navigator',
    'station',
    'watch'
);


--
-- Name: enum_skills_homepageBadgeTypes; Type: TYPE; Schema: public; Owner: -
--

CREATE TYPE public."enum_skills_homepageBadgeTypes" AS ENUM (
    'Kids',
    'New',
    'Puzzle',
    'Quest',
    'AliceChoice',
    'Helpful',
    'Sport',
    'Interesting',
    'Wonderful',
    'Tasty'
);


--
-- Name: enum_skills_monitoringType; Type: TYPE; Schema: public; Owner: -
--

CREATE TYPE public."enum_skills_monitoringType" AS ENUM (
    'monitored',
    'nonmonitored',
    'yandex',
    'justAI'
);


--
-- Name: enum_skills_requiredInterfaces; Type: TYPE; Schema: public; Owner: -
--

CREATE TYPE public."enum_skills_requiredInterfaces" AS ENUM (
    'browser',
    'screen'
);


--
-- Name: enum_skills_skillAccess; Type: TYPE; Schema: public; Owner: -
--

CREATE TYPE public."enum_skills_skillAccess" AS ENUM (
    'public',
    'hidden',
    'private'
);


--
-- Name: enum_skills_surfaceBlacklist; Type: TYPE; Schema: public; Owner: -
--

CREATE TYPE public."enum_skills_surfaceBlacklist" AS ENUM (
    'auto',
    'desktop',
    'mobile',
    'navigator',
    'station',
    'watch'
);


--
-- Name: enum_skills_surfaceWhitelist; Type: TYPE; Schema: public; Owner: -
--

CREATE TYPE public."enum_skills_surfaceWhitelist" AS ENUM (
    'auto',
    'desktop',
    'mobile',
    'navigator',
    'station',
    'watch'
    );


--
-- Name: enum_show_types; Type: TYPE; Schema: public; Owner: -
--
CREATE TYPE public."enum_show_types" AS ENUM (
    'morning'
    );



--
-- Name: get_heap_bloat_info(); Type: FUNCTION; Schema: public; Owner: -
--

/*CREATE FUNCTION public.get_heap_bloat_info() RETURNS TABLE(current_database name, schemaname name, tblname name, real_size text, extra_size text, extra_ratio double precision, fillfactor integer, bloat_size text, bloat_size_bytes bigint, bloat_ratio double precision, is_na boolean)
    LANGUAGE plpgsql SECURITY DEFINER
    AS $$
BEGIN
RETURN QUERY WITH s AS (
SELECT
        tbl.oid AS tblid,
	ns.nspname AS schemaname,
	tbl.relname AS tblname,
	tbl.reltuples,
        tbl.relpages AS heappages,
	COALESCE(toast.relpages, 0) AS toastpages,
        COALESCE(toast.reltuples, 0) AS toasttuples,
        COALESCE(substring(array_to_string(tbl.reloptions, ' ')
          FROM '%fillfactor=#"__#"%' FOR '#')::smallint, 100) AS fillfactor,
        current_setting('block_size')::numeric AS bs,
        CASE WHEN version()~'mingw32' OR version()~'64-bit|x86_64|ppc64|ia64|amd64'
	THEN 8
	ELSE 4
	END AS ma,
        24 AS page_hdr,
        23 + CASE WHEN MAX(coalesce(null_frac,0)) > 0
	THEN ( 7 + count(*) ) / 8
	ELSE 0::int
	END
          + CASE WHEN tbl.relhasoids
	THEN 4
	ELSE 0
	END AS tpl_hdr_size,
        sum( (1-coalesce(s.null_frac, 0)) * coalesce(s.avg_width, 1024) ) AS tpl_data_size,
        bool_or(att.atttypid = 'pg_catalog.name'::regtype) AS is_na
FROM pg_attribute AS att
        JOIN pg_class AS tbl ON att.attrelid = tbl.oid
        JOIN pg_namespace AS ns ON ns.oid = tbl.relnamespace
        JOIN pg_stats AS s ON s.schemaname=ns.nspname
          AND s.tablename = tbl.relname AND s.inherited=false AND s.attname=att.attname
        LEFT JOIN pg_class AS toast ON tbl.reltoastrelid = toast.oid
WHERE att.attnum > 0 AND NOT att.attisdropped
        AND tbl.relkind = 'r'
GROUP BY
	1,2,3,4,5,6,7,8,9,10, tbl.relhasoids
ORDER BY
	5 DESC
), s2 AS (
SELECT
      	( 4 + tpl_hdr_size + tpl_data_size + (2*ma)
        - CASE WHEN tpl_hdr_size%ma = 0
	THEN ma
	ELSE tpl_hdr_size%ma
	END
        - CASE WHEN ceil(tpl_data_size)::int%ma = 0
	THEN ma
	ELSE ceil(tpl_data_size)::int%ma
	END
      	) AS tpl_size,
	bs - page_hdr AS size_per_block,
	(heappages + toastpages) AS tblpages,
	heappages,
      	toastpages,
	reltuples,
	toasttuples,
	bs,
	page_hdr,
	tblid,
	s.schemaname,
	s.tblname,
	s.fillfactor,
	s.is_na
FROM s
), s3 AS (
SELECT
	ceil( reltuples / ( (bs-page_hdr)/tpl_size ) ) + ceil( toasttuples / 4 ) AS est_tblpages,
    	ceil( reltuples / ( (bs-page_hdr)*s2.fillfactor/(tpl_size*100) ) ) + ceil( toasttuples / 4 ) AS est_tblpages_ff,
    	s2.tblpages,
    	s2.fillfactor,
    	s2.bs,
    	s2.tblid,
    	s2.schemaname,
    	s2.tblname,
    	s2.heappages,
    	s2.toastpages,
    	s2.is_na
FROM s2
) SELECT
	current_database(),
	s3.schemaname,
	s3.tblname,
	pg_size_pretty(bs*s3.tblpages) AS real_size,
	pg_size_pretty(((s3.tblpages-est_tblpages)*bs)::bigint) AS extra_size,
  	CASE WHEN tblpages - est_tblpages > 0
    	THEN 100 * (s3.tblpages - est_tblpages)/s3.tblpages::float
    	ELSE 0
  	END AS extra_ratio,
	s3.fillfactor,
	pg_size_pretty(((s3.tblpages-est_tblpages_ff)*bs)::bigint) AS bloat_size,
  	((tblpages-est_tblpages_ff)*bs)::bigint bytes_bloat_size,
  	CASE WHEN s3.tblpages - est_tblpages_ff > 0
    	THEN 100 * (s3.tblpages - est_tblpages_ff)/s3.tblpages::float
    	ELSE 0
  END AS bloat_ratio, s3.is_na
  FROM s3;

END;
$$;


--
-- Name: get_index_bloat_info(); Type: FUNCTION; Schema: public; Owner: -
--

CREATE FUNCTION public.get_index_bloat_info() RETURNS TABLE(dbname name, schema_name name, table_name name, index_name name, bloat_pct numeric, bloat_bytes numeric, bloat_size text, total_bytes numeric, index_size text, table_bytes bigint, table_size text, index_scans bigint)
    LANGUAGE plpgsql SECURITY DEFINER
    AS $$
BEGIN

RETURN QUERY WITH btree_index_atts AS (
    SELECT
	nspname,
	relname,
	reltuples,
	relpages,
	indrelid,
	relam,
        regexp_split_to_table(indkey::text, ' ')::smallint AS attnum,
        indexrelid as index_oid
    FROM pg_index
    JOIN pg_class ON pg_class.oid=pg_index.indexrelid
    JOIN pg_namespace ON pg_namespace.oid = pg_class.relnamespace
    JOIN pg_am ON pg_class.relam = pg_am.oid
    WHERE pg_am.amname = 'btree'
    ),
index_item_sizes AS (
    SELECT
    	i.nspname,
    	i.relname,
	i.reltuples,
	i.relpages,
	i.relam,
    	s.starelid,
	a.attrelid AS table_oid,
	index_oid,
    	current_setting('block_size')::numeric AS bs,
    	/* MAXALIGN: 4 on 32bits, 8 on 64bits (and mingw32 ?) */
    	CASE
        WHEN version() ~ 'mingw32' OR version() ~ '64-bit'
	THEN 8
        ELSE 4
    	END AS maxalign,
    	24 AS pagehdr,
    	/* per tuple header: add index_attribute_bm if some cols are null-able */
    	CASE WHEN max(coalesce(s.stanullfrac,0)) = 0
	THEN 2
	ELSE 6
    	END AS index_tuple_hdr,
    	/* data len: we remove null values save space using it fractionnal part from stats */
    	SUM( (1-coalesce(s.stanullfrac, 0)) * coalesce(s.stawidth, 2048) ) AS nulldatawidth
    	FROM pg_attribute AS a
    	JOIN pg_statistic AS s ON s.starelid=a.attrelid AND s.staattnum = a.attnum
    	JOIN btree_index_atts AS i ON i.indrelid = a.attrelid AND a.attnum = i.attnum
    	WHERE
		a.attnum > 0
    	GROUP BY
		1, 2, 3, 4, 5, 6, 7, 8, 9
),
index_aligned AS (
    SELECT
	maxalign,
	bs,
	nspname,
	relname AS index_name,
	reltuples,
        relpages,
	relam,
	table_oid,
	index_oid,
      	( 2 + maxalign -
	CASE /* Add padding to the index tuple header to align on MAXALIGN */
            WHEN index_tuple_hdr%maxalign = 0
	    THEN maxalign
            ELSE index_tuple_hdr%maxalign
          END
        + nulldatawidth + maxalign -
	CASE /* Add padding to the data to align on MAXALIGN */
            WHEN nulldatawidth::integer%maxalign = 0 THEN maxalign
            ELSE nulldatawidth::integer%maxalign
          END
      	)::numeric AS nulldatahdrwidth,
	pagehdr
    FROM index_item_sizes AS s1
),
otta_calc AS (
  SELECT
	s2.bs,
	s2.nspname,
	s2.table_oid,
	s2.index_oid,
	s2.index_name,
	s2.relpages,
	COALESCE(CEIL((reltuples*(4+nulldatahdrwidth))/(bs-pagehdr::float)) +
      	CASE WHEN am.amname IN ('hash','btree')
	THEN 1
	ELSE 0
	END ,
	0 -- btree and hash have a metadata reserved block
    ) AS otta
  FROM index_aligned AS s2
    LEFT JOIN pg_am am ON s2.relam = am.oid
),
raw_bloat AS (
    SELECT
	current_database() AS dbname,
	nspname,
	c.relname AS table_name,
	sub.index_name,
        bs*(sub.relpages)::bigint AS totalbytes,
        CASE
            WHEN sub.relpages <= otta THEN 0
            ELSE bs*(sub.relpages-otta)::bigint END
            AS wastedbytes,
        CASE
            WHEN sub.relpages <= otta
            THEN 0
		ELSE bs*(sub.relpages-otta)::bigint * 100 / (bs*(sub.relpages)::bigint)
		END
        AS realbloat,
        pg_relation_size(sub.table_oid) AS table_bytes,
        stat.idx_scan AS index_scans
    FROM otta_calc AS sub
    JOIN pg_class AS c ON c.oid=sub.table_oid
    JOIN pg_stat_user_indexes AS stat ON sub.index_oid = stat.indexrelid
)
SELECT
	r.dbname AS database_name,
	nspname AS schema_name,
	r.table_name,
	r.index_name,
        round(realbloat, 1) AS bloat_pct,
        wastedbytes AS bloat_bytes,
	pg_size_pretty(wastedbytes::bigint) AS bloat_size,
        totalbytes AS index_bytes,
	pg_size_pretty(totalbytes::bigint) AS index_size,
        r.table_bytes,
	pg_size_pretty(r.table_bytes) AS table_size,
        r.index_scans
FROM raw_bloat r;
END;
$$;*/


--
-- Name: prepare_for_fts(text); Type: FUNCTION; Schema: public; Owner: -
--

CREATE FUNCTION public.prepare_for_fts(value text) RETURNS text
    LANGUAGE sql STABLE
    AS $$
            SELECT
                COALESCE(lower(regexp_replace(value, '[^0-9a-zA-ZА-Яа-яёЁ\s]', '', 'g') COLLATE "C.UTF-8"), '')
            $$;


--
-- Name: update_skills_rating(integer); Type: FUNCTION; Schema: public; Owner: -
--

CREATE FUNCTION public.update_skills_rating(min_reviews_count integer DEFAULT 5) RETURNS integer
    LANGUAGE plpgsql
    AS $$
DECLARE
  started   timestamptz := now();
  row_count integer;
BEGIN
  UPDATE skills s
  SET rating = ARRAY[q.s1, q.s2, q.s3, q.s4, q.s5]
  FROM (SELECT e."skillId"                        AS id,
               count(*) FILTER (WHERE rating = 1) AS s1,
               count(*) FILTER (WHERE rating = 2) AS s2,
               count(*) FILTER (WHERE rating = 3) AS s3,
               count(*) FILTER (WHERE rating = 4) AS s4,
               count(*) FILTER (WHERE rating = 5) AS s5
        FROM (SELECT r."skillId", r.rating
              FROM "userReviews" r
                     JOIN (SELECT u."skillId"
                           FROM "userReviews" u
                           WHERE "updatedAt" >= (SELECT start_time FROM jobs_history WHERE name = 'update_skills_rating'
                                                                                       AND success
                                                 UNION (SELECT to_timestamp(0))
                                                 ORDER BY start_time DESC
                                                 LIMIT 1)
                             AND "updatedAt" < started
                           GROUP BY u."skillId") q ON r."skillId" = q."skillId") e
        GROUP BY e."skillId"
        HAVING count(*) >= min_reviews_count) q
  WHERE s.id = q.id;

  GET DIAGNOSTICS row_count = ROW_COUNT;

  INSERT INTO jobs_history (name, start_time, end_time, success, row_count)
  VALUES ('update_skills_rating', started, now(), TRUE, row_count);

  RETURN row_count;
END;
$$;


--
-- Name: postgresdb; Type: SERVER; Schema: -; Owner: -
--

/*CREATE SERVER postgresdb FOREIGN DATA WRAPPER postgres_fdw OPTIONS (
    dbname 'postgres',
    host 'localhost',
    port '6432',
    updatable 'false'
);


--
-- Name: USER MAPPING public SERVER postgresdb; Type: USER MAPPING; Schema: -; Owner: -
--

CREATE USER MAPPING FOR public SERVER postgresdb;


SET default_tablespace = '';*/

SET default_with_oids = false;

--
-- Name: SequelizeMeta; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public."SequelizeMeta" (
    name character varying(255) NOT NULL
);


--
-- Name: alicePrizeNominees; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public."alicePrizeNominees" (
    id uuid NOT NULL,
    "skillId" uuid NOT NULL,
    "rotationWithdrawn" boolean DEFAULT false NOT NULL,
    "createdAt" timestamp with time zone NOT NULL,
    "updatedAt" timestamp with time zone NOT NULL
);


--
-- Name: apiCalls; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public."apiCalls" (
    id integer NOT NULL,
    "createdAt" timestamp with time zone NOT NULL,
    "updatedAt" timestamp with time zone NOT NULL,
    "userAgent" character varying(255) DEFAULT 'none'::character varying NOT NULL,
    method character varying(255) NOT NULL
);


--
-- Name: apiCalls_id_seq; Type: SEQUENCE; Schema: public; Owner: -
--

CREATE SEQUENCE public."apiCalls_id_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


--
-- Name: apiCalls_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: -
--

ALTER SEQUENCE public."apiCalls_id_seq" OWNED BY public."apiCalls".id;


--
-- Name: devicesTestingRecords; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public."devicesTestingRecords" (
    id uuid NOT NULL,
    "skillId" uuid NOT NULL,
    type "enum_devicesTestingRecords_type" NOT NULL,
    options jsonb,
    "createdAt" timestamp with time zone NOT NULL,
    "updatedAt" timestamp with time zone NOT NULL
);


--
-- Name: draftIntents; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public."draftIntents" (
    id uuid NOT NULL,
    "skillId" uuid NOT NULL,
    "createdAt" timestamp with time zone NOT NULL,
    "updatedAt" timestamp with time zone NOT NULL,
    "formName" text,
    "humanReadableName" text DEFAULT ''::text NOT NULL,
    "sourceText" text DEFAULT ''::text NOT NULL,
    "positiveTests" text[] DEFAULT ARRAY[]::text[] NOT NULL,
    "negativeTests" text[] DEFAULT ARRAY[]::text[] NOT NULL,
    base64 text,
    "isActivation" boolean DEFAULT false NOT NULL
);


--
-- Name: draftMarketDevices; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public."draftMarketDevices" (
    id uuid NOT NULL,
    "skillId" uuid NOT NULL,
    "createdAt" timestamp with time zone NOT NULL,
    "updatedAt" timestamp with time zone NOT NULL,
    "marketId" text NOT NULL,
    "testedWithAlice" boolean DEFAULT false NOT NULL,
    "isPublished" boolean DEFAULT false NOT NULL
);


--
-- Name: draftUserAgreements; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public."draftUserAgreements" (
    id uuid NOT NULL,
    "skillId" uuid NOT NULL,
    "order" integer DEFAULT 0 NOT NULL,
    name text NOT NULL,
    url text NOT NULL,
    "createdAt" timestamp with time zone NOT NULL,
    "updatedAt" timestamp with time zone NOT NULL
);


--
-- Name: drafts; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public.drafts (
    id uuid NOT NULL,
    "createdAt" timestamp with time zone NOT NULL,
    "updatedAt" timestamp with time zone NOT NULL,
    name text DEFAULT ''::text NOT NULL,
    logo character varying(255) DEFAULT ''::character varying NOT NULL,
    "activationPhrases" character varying(255)[] DEFAULT (ARRAY[]::character varying[])::character varying(255)[] NOT NULL,
    "backendSettings" json DEFAULT '{}'::json NOT NULL,
    "publishingSettings" json DEFAULT '{}'::json NOT NULL,
    status enum_drafts_status DEFAULT 'inDevelopment'::enum_drafts_status NOT NULL,
    "skillId" uuid NOT NULL,
    channel enum_drafts_channel DEFAULT 'aliceSkill'::enum_drafts_channel NOT NULL,
    "logoId" uuid,
    "deployedToOrganizationChats" boolean DEFAULT false NOT NULL,
    "deployedToAlice" boolean DEFAULT false NOT NULL,
    "hideInStore" boolean DEFAULT false NOT NULL,
    "noteForModerator" text DEFAULT ''::text NOT NULL,
    "exactSurfaces" "enum_drafts_exactSurfaces"[] DEFAULT '{}'::"enum_drafts_exactSurfaces"[] NOT NULL,
    "surfaceWhitelist" "enum_drafts_surfaceWhitelist"[] DEFAULT '{}'::"enum_drafts_surfaceWhitelist"[] NOT NULL,
    "surfaceBlacklist" "enum_drafts_surfaceBlacklist"[] DEFAULT '{}'::"enum_drafts_surfaceBlacklist"[] NOT NULL,
    "requiredInterfaces" "enum_drafts_requiredInterfaces"[] DEFAULT '{}'::"enum_drafts_requiredInterfaces"[] NOT NULL,
    voice text DEFAULT 'good_oksana'::text,
    "inflectedActivationPhrases" text[],
    "yaCloudGrant" boolean DEFAULT false NOT NULL,
    "reviewRequestedAt" timestamp with time zone,
    "approvedETag" text,
    "activationPhrasesCommonness" "enum_drafts_activationPhrasesCommonness"[],
    "activationPhrasesForceApproval" boolean[],
    "oauthAppId" uuid,
    "approvedPrivateETag" text,
    "startrekTicket" text,
    "samsaraTicket" integer,
    grammars jsonb,
    "grammarsBase64" text,
    "appMetricaApiKey" text,
    "isTrustedSmartHomeSkill" boolean DEFAULT false,
    "useStateStorage" boolean DEFAULT false,
    "rsyPlatformId" text,
    "nameTts" text,
    "customEntities" text,
    "skillAccess" "enum_drafts_skillAccess",
    "approvedSettingsETag" text,
    CONSTRAINT "drafts_hideInStore_skillAccess_check" CHECK ((((NOT "hideInStore") AND ("skillAccess" = 'public'::"enum_drafts_skillAccess")) OR ("hideInStore" AND ("skillAccess" = ANY ('{private,hidden}'::"enum_drafts_skillAccess"[]))))),
    CONSTRAINT is_trusted_smart_home_not_null_check CHECK (("isTrustedSmartHomeSkill" IS NOT NULL)),
    CONSTRAINT use_state_storage_not_null_check CHECK (("useStateStorage" IS NOT NULL))
);


--
-- Name: emailAlerts; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public."emailAlerts" (
    id uuid NOT NULL,
    type "enum_emailAlerts_type" NOT NULL,
    "createdAt" timestamp with time zone NOT NULL,
    "updatedAt" timestamp with time zone NOT NULL,
    "skillId" uuid NOT NULL
);


--
-- Name: favouriteSkills; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public."favouriteSkills" (
    id uuid NOT NULL,
    "userId" character varying(255) NOT NULL,
    "skillId" uuid NOT NULL,
    "createdAt" timestamp with time zone,
    "updatedAt" timestamp with time zone
);


--
-- Name: ferrymanJobs; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public."ferrymanJobs" (
    id uuid NOT NULL,
    type "enum_ferrymanJobs_type" NOT NULL,
    "createdAt" timestamp with time zone NOT NULL,
    "tableName" text NOT NULL,
    "ferrymanTimestamp" bigint,
    "ferrymanBatch" text,
    status "enum_ferrymanJobs_status" DEFAULT 'created'::"enum_ferrymanJobs_status" NOT NULL,
    "updatedAt" timestamp with time zone NOT NULL
);


--
-- Name: images; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public.images (
    "createdAt" timestamp with time zone NOT NULL,
    "updatedAt" timestamp with time zone NOT NULL,
    id uuid NOT NULL,
    "skillId" uuid NOT NULL,
    url text NOT NULL,
    "origUrl" text,
    type enum_images_type NOT NULL,
    meta json DEFAULT '{}'::json NOT NULL,
    size integer NOT NULL
);


--
-- Name: jobs_history; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public.jobs_history (
    name text NOT NULL,
    start_time timestamp with time zone,
    end_time timestamp with time zone,
    success boolean,
    row_count integer
);


--
-- Name: newsContents; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public."newsContents" (
    id uuid NOT NULL,
    "feedId" uuid NOT NULL,
    uid text DEFAULT ''::text NOT NULL,
    "pubDate" timestamp with time zone NOT NULL,
    title text DEFAULT ''::text NOT NULL,
    "streamUrl" text,
    "mainText" text,
    "soundId" text,
    "imageUrl" text,
    "detailsUrl" text,
    "detailsText" text
);


--
-- Name: newsFeeds; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public."newsFeeds" (
                                    id                      uuid NOT NULL,
                                    "skillId"               uuid NOT NULL,
                                    "createdAt"             timestamp with time zone NOT NULL,
                                    "updatedAt"             timestamp with time zone NOT NULL,
                                    preamble                text,
                                    name                    text DEFAULT ''::text NOT NULL,
                                    description             text,
                                    topic                   text DEFAULT ''::text NOT NULL,
                                    type                    "enum_newsFeeds_type" DEFAULT 'rss'::"enum_newsFeeds_type" NOT NULL,
                                    url                     text DEFAULT ''::text NOT NULL,
                                    "inflectedTopicPhrases" text[],
                                    "iconUrl"               text,
                                    enabled                 boolean DEFAULT true NOT NULL,
                                    depth                   integer DEFAULT 1 NOT NULL,
                                    "settingsTypes"         character varying(255)[] DEFAULT (ARRAY []::character varying[])::character varying(255)[] NOT NULL
);


--
-- Name: showFeeds; Type: TABLE; Schema: public; Owner: -
--
CREATE TABLE public."showFeeds"
(
    id          uuid                                               not null,
    name        text                                               not null,
    "nameTts"   text,
    description text                                               not null,
    "skillId"   uuid                                               not null,
    type        enum_show_types default 'morning'::enum_show_types not null,
    "onAir"     boolean         default true                       not null
);


--
-- Name: oauthApps; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public."oauthApps"
(
    id              uuid                 NOT NULL,
    "userId"        text                 NOT NULL,
    name            text                 NOT NULL,
    "socialAppName" text NOT NULL,
    "createdAt" timestamp with time zone NOT NULL,
    "updatedAt" timestamp with time zone NOT NULL,
    "deletedAt" timestamp with time zone
);


--
-- Name: oneTimeShares; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public."oneTimeShares" (
    id text NOT NULL,
    "skillId" uuid NOT NULL,
    "expiredAt" timestamp with time zone NOT NULL
);


--
-- Name: operations; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public.operations (
    id integer NOT NULL,
    "createdAt" timestamp with time zone NOT NULL,
    "updatedAt" timestamp with time zone NOT NULL,
    type enum_operations_type NOT NULL,
    comment text DEFAULT ''::text NOT NULL,
    "itemId" uuid,
    "userId" text
);


--
-- Name: operations_id_seq; Type: SEQUENCE; Schema: public; Owner: -
--

CREATE SEQUENCE public.operations_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


--
-- Name: operations_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: -
--

ALTER SEQUENCE public.operations_id_seq OWNED BY public.operations.id;


--
-- Name: phoneConfirmations; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public."phoneConfirmations" (
    id uuid NOT NULL,
    "phoneNumber" text NOT NULL,
    "skillId" uuid NOT NULL,
    "validBefore" timestamp with time zone NOT NULL,
    attempts integer DEFAULT 0 NOT NULL,
    code text,
    "createdAt" timestamp with time zone,
    "updatedAt" timestamp with time zone
);


--
-- Name: publishedIntents; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public."publishedIntents" (
    id uuid NOT NULL,
    "skillId" uuid NOT NULL,
    "createdAt" timestamp with time zone NOT NULL,
    "updatedAt" timestamp with time zone NOT NULL,
    "formName" text,
    "humanReadableName" text DEFAULT ''::text NOT NULL,
    "sourceText" text DEFAULT ''::text NOT NULL,
    "positiveTests" text[] DEFAULT ARRAY[]::text[] NOT NULL,
    "negativeTests" text[] DEFAULT ARRAY[]::text[] NOT NULL,
    base64 text,
    "isActivation" boolean DEFAULT false NOT NULL
);


--
-- Name: publishedMarketDevices; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public."publishedMarketDevices" (
    id uuid NOT NULL,
    "skillId" uuid NOT NULL,
    "createdAt" timestamp with time zone NOT NULL,
    "updatedAt" timestamp with time zone NOT NULL,
    "marketId" text NOT NULL,
    "testedWithAlice" boolean DEFAULT false NOT NULL
);


--
-- Name: publishedUserAgreements; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public."publishedUserAgreements" (
    id uuid NOT NULL,
    "skillId" uuid NOT NULL,
    "order" integer DEFAULT 0 NOT NULL,
    name text NOT NULL,
    url text NOT NULL,
    "createdAt" timestamp with time zone NOT NULL,
    "updatedAt" timestamp with time zone NOT NULL
);


--
-- Name: repl_mon; Type: FOREIGN TABLE; Schema: public; Owner: -
--

/*CREATE FOREIGN TABLE public.repl_mon (
    ts timestamp with time zone,
    location text,
    replics integer,
    master text
)
SERVER postgresdb
OPTIONS (
    schema_name 'public',
    table_name 'repl_mon'
);*/


--
-- Name: skillCollections; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public."skillCollections" (
    id text NOT NULL,
    "skillSlugs" text[] NOT NULL,
    "categorySlug" text,
    description text
);


--
-- Name: skill_user_share; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public.skill_user_share (
    id uuid NOT NULL,
    user_id character varying(255) NOT NULL,
    skill_id uuid NOT NULL,
    "createdAt" timestamp with time zone,
    "updatedAt" timestamp with time zone
);


--
-- Name: skills; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public.skills (
    id uuid NOT NULL,
    "createdAt" timestamp with time zone NOT NULL,
    "updatedAt" timestamp with time zone NOT NULL,
    "deletedAt" timestamp with time zone,
    name text DEFAULT ''::text NOT NULL,
    slug character varying(255) DEFAULT ''::character varying NOT NULL,
    logo character varying(255) DEFAULT ''::character varying NOT NULL,
    "activationPhrases" character varying(255)[] DEFAULT (ARRAY[]::character varying[])::character varying(255)[] NOT NULL,
    "backendSettings" json DEFAULT '{}'::json NOT NULL,
    "publishingSettings" json DEFAULT '{}'::json NOT NULL,
    "onAir" boolean DEFAULT false NOT NULL,
    salt uuid NOT NULL,
    "useZora" boolean DEFAULT true NOT NULL,
    "userId" character varying(255) NOT NULL,
    channel enum_skills_channel DEFAULT 'aliceSkill'::enum_skills_channel NOT NULL,
    "exposeInternalFlags" boolean DEFAULT false NOT NULL,
    "bannedUntil" timestamp with time zone,
    "logoId" uuid,
    "isRecommended" boolean,
    "openInNewTab" boolean DEFAULT true NOT NULL,
    "botGuid" character varying(255) DEFAULT NULL::character varying,
    look character varying(255) DEFAULT 'external'::character varying NOT NULL,
    voice character varying(255) DEFAULT 'good_oksana'::character varying,
    "isVip" boolean DEFAULT false NOT NULL,
    "hideInStore" boolean DEFAULT false NOT NULL,
    "catalogRank" integer,
    "monitoringType" "enum_skills_monitoringType" DEFAULT 'nonmonitored'::"enum_skills_monitoringType" NOT NULL,
    "exactSurfaces" "enum_skills_exactSurfaces"[] DEFAULT '{}'::"enum_skills_exactSurfaces"[] NOT NULL,
    "surfaceWhitelist" "enum_skills_surfaceWhitelist"[] DEFAULT '{}'::"enum_skills_surfaceWhitelist"[] NOT NULL,
    "surfaceBlacklist" "enum_skills_surfaceBlacklist"[] DEFAULT '{}'::"enum_skills_surfaceBlacklist"[] NOT NULL,
    "requiredInterfaces" "enum_skills_requiredInterfaces"[] DEFAULT '{}'::"enum_skills_requiredInterfaces"[] NOT NULL,
    "useNLU" boolean DEFAULT false NOT NULL,
    rating integer[] DEFAULT ARRAY[0, 0, 0, 0, 0],
    "developerType" text DEFAULT 'external'::text,
    score real,
    "responseTime" integer,
    "inflectedActivationPhrases" text[],
    "yaCloudGrant" boolean DEFAULT false NOT NULL,
    "enableVoiceActivation" boolean DEFAULT true NOT NULL,
    "activationPhrasesCommonness" "enum_skills_activationPhrasesCommonness"[],
    "activationPhrasesForceApproval" boolean[],
    "oauthAppId" uuid,
    "notificationSettings" jsonb DEFAULT '{"options": {"newChat": ["email"], "noAnswer": ["email"], "noFastAnswer": ["email"], "noAnswerWeekReport": ["email"], "noFastAnswerWeekReport": ["email"]}}'::jsonb NOT NULL,
    "automaticIsRecommended" boolean,
    "isTrustedSmartHomeSkill" boolean,
    "recommendationMarkupRequestedAt" timestamp with time zone,
    "featureFlags" text[],
    "persistentUserIdSalt" uuid DEFAULT uuid_generate_v4() NOT NULL,
    "donationSettings" jsonb,
    grammars jsonb,
    "grammarsBase64" text,
    "appMetricaApiKey" text,
    "firstPublishedAt" timestamp with time zone,
    "useStateStorage" boolean DEFAULT false,
    "rsyPlatformId" text,
    "alicePrizeNomination" "enum_skills_alicePrizeNomination",
    "alicePrizeRecievedAt" timestamp with time zone,
    "nameTts" text,
    "customEntities" text,
    "skillAccess" "enum_skills_skillAccess",
    "publicShareKey" text,
    "lastPublishedAt" timestamp with time zone,
    tags character varying(255)[],
    "editorDescription" text,
    "editorName" text,
    "homepageBadgeTypes" "enum_skills_homepageBadgeTypes"[],
    "safeForKids" boolean,
    CONSTRAINT "persistentUserIdSalt_notnull" CHECK (("persistentUserIdSalt" IS NOT NULL)),
    CONSTRAINT "skills_hideInStore_skillAccess_check" CHECK ((((NOT "hideInStore") AND ("skillAccess" = 'public'::"enum_skills_skillAccess")) OR ("hideInStore" AND ("skillAccess" = ANY ('{private,hidden}'::"enum_skills_skillAccess"[]))))),
    CONSTRAINT use_state_storage_not_null_check CHECK (("useStateStorage" IS NOT NULL))
);


--
-- Name: skillsCrypto; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public."skillsCrypto" (
    "skillId" uuid NOT NULL,
    "publicKey" text NOT NULL,
    "privateKey" text
);


--
-- Name: sounds; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public.sounds (
    id uuid NOT NULL,
    "skillId" uuid NOT NULL,
    "originalSize" integer,
    size integer,
    "originalMeta" jsonb,
    meta jsonb,
    "originalPath" text NOT NULL,
    path text,
    "processedAt" timestamp with time zone,
    "processTime" integer,
    error jsonb,
    retries integer DEFAULT 1 NOT NULL,
    "lastAttempt" timestamp with time zone,
    "createdAt" timestamp with time zone NOT NULL,
    "updatedAt" timestamp with time zone NOT NULL,
    "deletedAt" timestamp with time zone,
    "originalName" text,
    "maxDurationSec" integer
);


--
-- Name: subscriptions; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public.subscriptions (
    id uuid NOT NULL,
    "userId" text,
    ip text NOT NULL,
    service text NOT NULL,
    type text NOT NULL,
    "createdAt" timestamp with time zone NOT NULL,
    "updatedAt" timestamp with time zone NOT NULL
);


--
-- Name: tycoonRubrics; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public."tycoonRubrics" (
    permalink character varying(255) NOT NULL,
    name character varying(255) NOT NULL,
    "createdAt" timestamp with time zone NOT NULL,
    "updatedAt" timestamp with time zone NOT NULL
);


--
-- Name: userReviews; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public."userReviews" (
    "skillId" uuid NOT NULL,
    "userId" text NOT NULL,
    rating integer NOT NULL,
    "reviewText" text DEFAULT ''::text NOT NULL,
    "quickAnswers" text[] DEFAULT ARRAY[]::text[] NOT NULL,
    "createdAt" timestamp with time zone NOT NULL,
    "updatedAt" timestamp with time zone NOT NULL,
    "deletedAt" timestamp with time zone
);


--
-- Name: users; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public.users (
    id character varying(255) NOT NULL,
    "createdAt" timestamp with time zone NOT NULL,
    "updatedAt" timestamp with time zone NOT NULL,
    name character varying(255) NOT NULL,
    "isAdmin" boolean DEFAULT false NOT NULL,
    "isBanned" boolean DEFAULT false NOT NULL,
    "hasSubscription" boolean DEFAULT true NOT NULL,
    "featureFlags" jsonb DEFAULT '{}'::jsonb,
    "resourcesQuota" jsonb DEFAULT '{}'::jsonb,
    roles text[] DEFAULT ARRAY[]::text[],
    "yandexTeamLogin" text,
    "bannedNews" text[] DEFAULT ARRAY[]::text[],
    "hasNewsSubscription" boolean DEFAULT false
);


--
-- Name: apiCalls id; Type: DEFAULT; Schema: public; Owner: -
--

ALTER TABLE ONLY public."apiCalls" ALTER COLUMN id SET DEFAULT nextval('"apiCalls_id_seq"'::regclass);


--
-- Name: operations id; Type: DEFAULT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.operations ALTER COLUMN id SET DEFAULT nextval('operations_id_seq'::regclass);


--
-- Name: SequelizeMeta SequelizeMeta_pkey; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public."SequelizeMeta"
    ADD CONSTRAINT "SequelizeMeta_pkey" PRIMARY KEY (name);


--
-- Name: alicePrizeNominees alicePrizeNominees_pkey; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public."alicePrizeNominees"
    ADD CONSTRAINT "alicePrizeNominees_pkey" PRIMARY KEY (id);


--
-- Name: apiCalls apiCalls_pkey; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public."apiCalls"
    ADD CONSTRAINT "apiCalls_pkey" PRIMARY KEY (id);


--
-- Name: devicesTestingRecords devicesTestingRecords_pkey; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public."devicesTestingRecords"
    ADD CONSTRAINT "devicesTestingRecords_pkey" PRIMARY KEY (id);


--
-- Name: devicesTestingRecords devicesTestingRecords_skillId_key; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public."devicesTestingRecords"
    ADD CONSTRAINT "devicesTestingRecords_skillId_key" UNIQUE ("skillId");


--
-- Name: draftIntents draftIntents_pkey; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public."draftIntents"
    ADD CONSTRAINT "draftIntents_pkey" PRIMARY KEY (id);


--
-- Name: draftMarketDevices draftMarketDevices_pkey; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public."draftMarketDevices"
    ADD CONSTRAINT "draftMarketDevices_pkey" PRIMARY KEY (id);


--
-- Name: draftUserAgreements draftUserAgreements_pkey; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public."draftUserAgreements"
    ADD CONSTRAINT "draftUserAgreements_pkey" PRIMARY KEY (id);


--
-- Name: drafts drafts_pkey; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.drafts
    ADD CONSTRAINT drafts_pkey PRIMARY KEY (id);


--
-- Name: draftUserAgreements draftuseragreements_skillid_name_uniq; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public."draftUserAgreements"
    ADD CONSTRAINT draftuseragreements_skillid_name_uniq UNIQUE ("skillId", name) DEFERRABLE INITIALLY DEFERRED;


--
-- Name: draftUserAgreements draftuseragreements_skillid_order_uniq; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public."draftUserAgreements"
    ADD CONSTRAINT draftuseragreements_skillid_order_uniq UNIQUE ("skillId", "order") DEFERRABLE INITIALLY DEFERRED;


--
-- Name: draftUserAgreements draftuseragreements_skillid_url_uniq; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public."draftUserAgreements"
    ADD CONSTRAINT draftuseragreements_skillid_url_uniq UNIQUE ("skillId", url) DEFERRABLE INITIALLY DEFERRED;


--
-- Name: emailAlerts emailAlerts_pkey; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public."emailAlerts"
    ADD CONSTRAINT "emailAlerts_pkey" PRIMARY KEY (id);


--
-- Name: favouriteSkills favouriteSkills_pkey; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public."favouriteSkills"
    ADD CONSTRAINT "favouriteSkills_pkey" PRIMARY KEY (id);


--
-- Name: ferrymanJobs ferrymanJobs_pkey; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public."ferrymanJobs"
    ADD CONSTRAINT "ferrymanJobs_pkey" PRIMARY KEY (id);


--
-- Name: images images_pkey; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.images
    ADD CONSTRAINT images_pkey PRIMARY KEY (id);


--
-- Name: newsContents newsContents_pkey; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public."newsContents"
    ADD CONSTRAINT "newsContents_pkey" PRIMARY KEY (id);


--
-- Name: newsFeeds newsFeeds_pkey; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public."newsFeeds"
    ADD CONSTRAINT "newsFeeds_pkey" PRIMARY KEY (id);


--
-- Name: showFeeds showFeeds_pkey; Type: CONSTRAINT; Schema: public; Owner: -
--
ALTER TABLE ONLY public."showFeeds"
    ADD CONSTRAINT "showFeeds_pkey" PRIMARY KEY (id);


--
-- Name: oauthApps oauthApps_pkey; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public."oauthApps"
    ADD CONSTRAINT "oauthApps_pkey" PRIMARY KEY (id);


--
-- Name: oneTimeShares oneTimeShares_pkey; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public."oneTimeShares"
    ADD CONSTRAINT "oneTimeShares_pkey" PRIMARY KEY (id);


--
-- Name: operations operations_pkey; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.operations
    ADD CONSTRAINT operations_pkey PRIMARY KEY (id);


--
-- Name: phoneConfirmations phoneConfirmations_pkey; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public."phoneConfirmations"
    ADD CONSTRAINT "phoneConfirmations_pkey" PRIMARY KEY (id);


--
-- Name: publishedIntents publishedIntents_pkey; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public."publishedIntents"
    ADD CONSTRAINT "publishedIntents_pkey" PRIMARY KEY (id);


--
-- Name: publishedMarketDevices publishedMarketDevices_pkey; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public."publishedMarketDevices"
    ADD CONSTRAINT "publishedMarketDevices_pkey" PRIMARY KEY (id);


--
-- Name: publishedUserAgreements publishedUserAgreements_pkey; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public."publishedUserAgreements"
    ADD CONSTRAINT "publishedUserAgreements_pkey" PRIMARY KEY (id);


--
-- Name: publishedUserAgreements publisheduseragreements_skillid_name_uniq; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public."publishedUserAgreements"
    ADD CONSTRAINT publisheduseragreements_skillid_name_uniq UNIQUE ("skillId", name) DEFERRABLE INITIALLY DEFERRED;


--
-- Name: publishedUserAgreements publisheduseragreements_skillid_order_uniq; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public."publishedUserAgreements"
    ADD CONSTRAINT publisheduseragreements_skillid_order_uniq UNIQUE ("skillId", "order") DEFERRABLE INITIALLY DEFERRED;


--
-- Name: publishedUserAgreements publisheduseragreements_skillid_url_uniq; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public."publishedUserAgreements"
    ADD CONSTRAINT publisheduseragreements_skillid_url_uniq UNIQUE ("skillId", url) DEFERRABLE INITIALLY DEFERRED;


--
-- Name: skillCollections skillCollections_pkey; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public."skillCollections"
    ADD CONSTRAINT "skillCollections_pkey" PRIMARY KEY (id);


--
-- Name: skill_user_share skill_user_share_pkey; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.skill_user_share
    ADD CONSTRAINT skill_user_share_pkey PRIMARY KEY (id);


--
-- Name: skillsCrypto skillsCrypto_pkey; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public."skillsCrypto"
    ADD CONSTRAINT "skillsCrypto_pkey" PRIMARY KEY ("skillId");


--
-- Name: skills skills_pkey; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.skills
    ADD CONSTRAINT skills_pkey PRIMARY KEY (id);


--
-- Name: sounds sounds_pkey; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.sounds
    ADD CONSTRAINT sounds_pkey PRIMARY KEY (id);


--
-- Name: subscriptions subscriptions_pkey; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.subscriptions
    ADD CONSTRAINT subscriptions_pkey PRIMARY KEY (id);


--
-- Name: tycoonRubrics tycoonRubrics_pkey; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public."tycoonRubrics"
    ADD CONSTRAINT "tycoonRubrics_pkey" PRIMARY KEY (permalink);


--
-- Name: userReviews userReviews_pkey; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public."userReviews"
    ADD CONSTRAINT "userReviews_pkey" PRIMARY KEY ("skillId", "userId");


--
-- Name: users users_pkey; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.users
    ADD CONSTRAINT users_pkey PRIMARY KEY (id);


--
-- Name: alice_prize_nomination_date_idx; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX alice_prize_nomination_date_idx ON public.skills USING btree ("alicePrizeRecievedAt", "alicePrizeNomination");


--
-- Name: draft_intents_skill_id; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX draft_intents_skill_id ON public."draftIntents" USING btree ("skillId");


--
-- Name: draft_intents_skill_id_form_name; Type: INDEX; Schema: public; Owner: -
--

CREATE UNIQUE INDEX draft_intents_skill_id_form_name ON public."draftIntents" USING btree ("skillId", "formName");


--
-- Name: draft_market_devices_market_id; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX draft_market_devices_market_id ON public."draftMarketDevices" USING btree ("marketId");


--
-- Name: draft_market_devices_skill_id; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX draft_market_devices_skill_id ON public."draftMarketDevices" USING btree ("skillId");


--
-- Name: draft_market_devices_skill_id_market_id; Type: INDEX; Schema: public; Owner: -
--

CREATE UNIQUE INDEX draft_market_devices_skill_id_market_id ON public."draftMarketDevices" USING btree ("skillId", "marketId");


--
-- Name: draft_user_agreements_skill_id_order_created_at; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX draft_user_agreements_skill_id_order_created_at ON public."draftUserAgreements" USING btree ("skillId", "order", "createdAt");


--
-- Name: drafts_activation_phrases_idx; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX drafts_activation_phrases_idx ON public.drafts USING gin ("activationPhrases") WHERE (status = 'deployRequested'::enum_drafts_status);


--
-- Name: drafts_backend_settings_flash_briefing_type_idx; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX drafts_backend_settings_flash_briefing_type_idx ON public.drafts USING btree ((("backendSettings" #>> '{flashBriefingType}'::text[])));


--
-- Name: drafts_backend_settings_jivosite_id_idx; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX drafts_backend_settings_jivosite_id_idx ON public.drafts USING btree ((("backendSettings" #>> '{jivositeId}'::text[])));


--
-- Name: drafts_hide_in_store; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX drafts_hide_in_store ON public.drafts USING btree ("hideInStore");


--
-- Name: drafts_inflected_activation_phrases_idx; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX drafts_inflected_activation_phrases_idx ON public.drafts USING gin ("inflectedActivationPhrases") WHERE (status = 'deployRequested'::enum_drafts_status);


--
-- Name: drafts_logo_id; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX drafts_logo_id ON public.drafts USING btree ("logoId");


--
-- Name: drafts_name_idx; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX drafts_name_idx ON public.drafts USING btree (upper((name))) WHERE (status = 'deployRequested'::enum_drafts_status);


--
-- Name: drafts_provider_id_idx; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX drafts_provider_id_idx ON public.drafts USING btree (upper(("backendSettings" ->> ('jivositeId'::text)))) WHERE (status = 'deployRequested'::enum_drafts_status);


--
-- Name: drafts_skillid_fkey; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX drafts_skillid_fkey ON public.drafts USING btree ("skillId");


--
-- Name: drafts_startrek_ticket_idx; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX drafts_startrek_ticket_idx ON public.drafts USING btree ("startrekTicket");


--
-- Name: drafts_status_updated_at; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX drafts_status_updated_at ON public.drafts USING btree (status, "updatedAt");


--
-- Name: email_alerts_skill_id_type_created_at; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX email_alerts_skill_id_type_created_at ON public."emailAlerts" USING btree ("skillId", type, "createdAt");


--
-- Name: favourite_skills_user_created_at_desc; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX favourite_skills_user_created_at_desc ON public."favouriteSkills" USING btree ("userId", "createdAt" DESC);


--
-- Name: favourite_skills_user_id_skill_id; Type: INDEX; Schema: public; Owner: -
--

CREATE UNIQUE INDEX favourite_skills_user_id_skill_id ON public."favouriteSkills" USING btree ("userId", "skillId");


--
-- Name: ferryman_jobs_type_status_created_at; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX ferryman_jobs_type_status_created_at ON public."ferrymanJobs" USING btree (type, status, "createdAt");


--
-- Name: ferryman_jobs_type_updated_at; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX ferryman_jobs_type_updated_at ON public."ferrymanJobs" USING btree (type, "updatedAt");


--
-- Name: homepage_badges_idx; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX homepage_badges_idx ON public.skills USING gin ("homepageBadgeTypes") WHERE (("onAir" = true) AND ("deletedAt" IS NULL) AND ("skillAccess" = 'public'::"enum_skills_skillAccess"));


--
-- Name: images_list_idx; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX images_list_idx ON public.images USING btree ("skillId", type, "createdAt" DESC);


--
-- Name: images_skill_id_type; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX images_skill_id_type ON public.images USING btree ("skillId", type);


--
-- Name: images_url; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX images_url ON public.images USING btree (url);


--
-- Name: news_feeds_by_feed_id_pub_date_desc; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX news_feeds_by_feed_id_pub_date_desc ON public."newsContents" USING btree ("feedId", "pubDate" DESC);


--
-- Name: news_feeds_inflected_topic_phrases_idx; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX news_feeds_inflected_topic_phrases_idx ON public."newsFeeds" USING gin ("inflectedTopicPhrases") WHERE (enabled = true);


--
-- Name: news_feeds_settings_types; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX news_feeds_settings_types ON public."newsFeeds" USING gin ("settingsTypes");


--
-- Name: news_feeds_skill_id; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX news_feeds_skill_id ON public."newsFeeds" USING btree ("skillId");


--
-- Name: news_feeds_skill_id_topic_enabled; Type: INDEX; Schema: public; Owner: -
--

CREATE UNIQUE INDEX news_feeds_skill_id_topic_enabled ON public."newsFeeds" USING btree ("skillId", topic, enabled);


--
-- Name: oauth_apps_user_id; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX oauth_apps_user_id ON public."oauthApps" USING btree ("userId");


--
-- Name: operations_item_id; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX operations_item_id ON public.operations USING btree ("itemId");


--
-- Name: operations_type_created_at; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX operations_type_created_at ON public.operations USING btree (type, "createdAt");


--
-- Name: published_intents_skill_id; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX published_intents_skill_id ON public."publishedIntents" USING btree ("skillId");


--
-- Name: published_intents_skill_id_form_name; Type: INDEX; Schema: public; Owner: -
--

CREATE UNIQUE INDEX published_intents_skill_id_form_name ON public."publishedIntents" USING btree ("skillId", "formName");


--
-- Name: published_market_devices_market_id; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX published_market_devices_market_id ON public."publishedMarketDevices" USING btree ("marketId");


--
-- Name: published_market_devices_skill_id; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX published_market_devices_skill_id ON public."publishedMarketDevices" USING btree ("skillId");


--
-- Name: published_market_devices_skill_id_market_id; Type: INDEX; Schema: public; Owner: -
--

CREATE UNIQUE INDEX published_market_devices_skill_id_market_id ON public."publishedMarketDevices" USING btree ("skillId", "marketId");


--
-- Name: published_user_agreements_skill_id_order_created_at; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX published_user_agreements_skill_id_order_created_at ON public."publishedUserAgreements" USING btree ("skillId", "order", "createdAt");


--
-- Name: reviews_last_with_text; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX reviews_last_with_text ON public."userReviews" USING btree ("deletedAt", "skillId", "updatedAt" DESC) WHERE ("reviewText" <> ''::text);


--
-- Name: skill_user_share_createdAt_idx; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX "skill_user_share_createdAt_idx" ON public.skill_user_share USING btree ("createdAt" DESC);


--
-- Name: skill_user_share_skillid_idx; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX skill_user_share_skillid_idx ON public.skill_user_share USING btree (skill_id);


--
-- Name: skill_user_share_user_id_skill_id; Type: INDEX; Schema: public; Owner: -
--

CREATE UNIQUE INDEX skill_user_share_user_id_skill_id ON public.skill_user_share USING btree (user_id, skill_id);


--
-- Name: skills_activation_phrases_idx; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX skills_activation_phrases_idx ON public.skills USING gin ("activationPhrases") WHERE (("onAir" = true) AND ("deletedAt" IS NULL));


--
-- Name: skills_alice_inflected_activation_phrases_idx; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX skills_alice_inflected_activation_phrases_idx ON public.skills USING gin ("inflectedActivationPhrases") WHERE ((channel = 'aliceSkill'::enum_skills_channel) AND ("onAir" = true) AND ("deletedAt" IS NULL));


--
-- Name: skills_backend_settings_flash_briefing_type_idx; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX skills_backend_settings_flash_briefing_type_idx ON public.skills USING btree ((("backendSettings" #>> '{flashBriefingType}'::text[])));


--
-- Name: skills_backend_settings_jivosite_id_idx; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX skills_backend_settings_jivosite_id_idx ON public.skills USING btree ((("backendSettings" #>> '{jivositeId}'::text[])));


--
-- Name: skills_catalog_rank; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX skills_catalog_rank ON public.skills USING btree ("catalogRank");


--
-- Name: skills_hide_in_store; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX skills_hide_in_store ON public.skills USING btree ("hideInStore");


--
-- Name: skills_inflected_activation_phrases_idx; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX skills_inflected_activation_phrases_idx ON public.skills USING gin ("inflectedActivationPhrases") WHERE (("onAir" = true) AND ("deletedAt" IS NULL));


--
-- Name: skills_logo_id; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX skills_logo_id ON public.skills USING btree ("logoId");


--
-- Name: skills_name_idx; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX skills_name_idx ON public.skills USING btree (upper((name))) WHERE (("onAir" = true) AND ("deletedAt" IS NULL));


--
-- Name: skills_provider_id_idx; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX skills_provider_id_idx ON public.skills USING btree (upper(("backendSettings" ->> ('jivositeId'::text)))) WHERE (("onAir" = true) AND ("deletedAt" IS NULL));


--
-- Name: skills_publishing_settings_category_idx; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX skills_publishing_settings_category_idx ON public.skills USING btree ((("publishingSettings" #>> '{category}'::text[])));


--
-- Name: skills_recommendationMarkupRequestedAt_idx; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX "skills_recommendationMarkupRequestedAt_idx" ON public.skills USING btree ("createdAt") WHERE (("onAir" = true) AND ("hideInStore" = false) AND ("deletedAt" IS NULL) AND (channel = 'aliceSkill'::enum_skills_channel) AND ("recommendationMarkupRequestedAt" IS NULL));


--
-- Name: skills_share_key_idx; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX skills_share_key_idx ON public.skills USING btree ("publicShareKey") WHERE (channel <> 'organizationChat'::enum_skills_channel);


--
-- Name: skills_slug_idx; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX skills_slug_idx ON public.skills USING btree (slug);


--
-- Name: skills_smart_home_idx; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX skills_smart_home_idx ON public.skills USING btree ("isTrustedSmartHomeSkill" DESC NULLS LAST, name, score DESC NULLS LAST, "createdAt") WHERE (("onAir" = true) AND ("deletedAt" IS NULL) AND (channel = 'smartHome'::enum_skills_channel));


--
-- Name: skills_smart_home_idx_2; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX skills_smart_home_idx_2 ON public.skills USING btree ("logoId", id) WHERE (("onAir" = true) AND (channel = 'smartHome'::enum_skills_channel));


--
-- Name: skills_store_listing2_idx; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX skills_store_listing2_idx ON public.skills USING btree ((("publishingSettings" #>> '{category}'::text[])), look DESC, channel, score DESC, "catalogRank", "createdAt") WHERE (("deletedAt" IS NULL) AND ("onAir" = true) AND ("hideInStore" = false));


--
-- Name: skills_store_listing2_no_category_idx; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX skills_store_listing2_no_category_idx ON public.skills USING btree (look DESC, channel, score DESC, "catalogRank", "createdAt") WHERE (("deletedAt" IS NULL) AND ("onAir" = true) AND ("hideInStore" = false));


--
-- Name: skills_store_listing4_idx; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX skills_store_listing4_idx ON public.skills USING btree ((("publishingSettings" #>> '{category}'::text[])), score DESC NULLS LAST, "createdAt") WHERE (("onAir" = true) AND ("hideInStore" = false) AND ("deletedAt" IS NULL) AND ((channel = 'aliceSkill'::enum_skills_channel) OR (score > (0)::double precision)));


--
-- Name: skills_store_similar_idx; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX skills_store_similar_idx ON public.skills USING btree (score DESC NULLS LAST, "createdAt") WHERE (("onAir" = true) AND ("hideInStore" = false) AND ("deletedAt" IS NULL) AND ((channel = 'aliceSkill'::enum_skills_channel) OR (score > (0)::double precision)));


--
-- Name: skills_user_id; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX skills_user_id ON public.skills USING btree ("userId");


--
-- Name: sounds_quota_size; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX sounds_quota_size ON public.sounds USING btree ("skillId", "deletedAt");


--
-- Name: tags_idx; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX tags_idx ON public.skills USING gin (tags) WHERE (("onAir" = true) AND ("deletedAt" IS NULL) AND ("skillAccess" = 'public'::"enum_skills_skillAccess"));


--
-- Name: user_reviews_skill_id_user_id_deleted_at; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX user_reviews_skill_id_user_id_deleted_at ON public."userReviews" USING btree ("skillId", "userId", "deletedAt");


--
-- Name: user_reviews_updated_at_idx; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX user_reviews_updated_at_idx ON public."userReviews" USING btree ("updatedAt");


--
-- Name: alicePrizeNominees alicePrizeNominees_skillId_fkey; Type: FK CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public."alicePrizeNominees"
    ADD CONSTRAINT "alicePrizeNominees_skillId_fkey" FOREIGN KEY ("skillId") REFERENCES skills(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: devicesTestingRecords devicesTestingRecords_skillId_fkey; Type: FK CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public."devicesTestingRecords"
    ADD CONSTRAINT "devicesTestingRecords_skillId_fkey" FOREIGN KEY ("skillId") REFERENCES skills(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: draftIntents draftIntents_skillId_fkey; Type: FK CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public."draftIntents"
    ADD CONSTRAINT "draftIntents_skillId_fkey" FOREIGN KEY ("skillId") REFERENCES skills(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: draftMarketDevices draftMarketDevices_skillId_fkey; Type: FK CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public."draftMarketDevices"
    ADD CONSTRAINT "draftMarketDevices_skillId_fkey" FOREIGN KEY ("skillId") REFERENCES skills(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: draftUserAgreements draftUserAgreements_skillId_fkey; Type: FK CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public."draftUserAgreements"
    ADD CONSTRAINT "draftUserAgreements_skillId_fkey" FOREIGN KEY ("skillId") REFERENCES skills(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: drafts drafts_logoId_fkey; Type: FK CONSTRAINT; Schema: public; Owner: -
--

--ALTER TABLE ONLY public.drafts
--    ADD CONSTRAINT "drafts_logoId_fkey" FOREIGN KEY ("logoId") REFERENCES images(id) ON UPDATE CASCADE ON DELETE SET NULL;


--
-- Name: drafts drafts_oauthAppId_fkey; Type: FK CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.drafts
    ADD CONSTRAINT "drafts_oauthAppId_fkey" FOREIGN KEY ("oauthAppId") REFERENCES "oauthApps"(id);


--
-- Name: drafts drafts_skillId_fkey; Type: FK CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.drafts
    ADD CONSTRAINT "drafts_skillId_fkey" FOREIGN KEY ("skillId") REFERENCES skills(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: emailAlerts emailAlerts_skillId_fkey; Type: FK CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public."emailAlerts"
    ADD CONSTRAINT "emailAlerts_skillId_fkey" FOREIGN KEY ("skillId") REFERENCES skills(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: favouriteSkills favouriteSkills_skillId_fkey; Type: FK CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public."favouriteSkills"
    ADD CONSTRAINT "favouriteSkills_skillId_fkey" FOREIGN KEY ("skillId") REFERENCES skills(id);


--
-- Name: images images_skillId_fkey; Type: FK CONSTRAINT; Schema: public; Owner: -
--

--ALTER TABLE ONLY public.images
--    ADD CONSTRAINT "images_skillId_fkey" FOREIGN KEY ("skillId") REFERENCES skills(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: newsContents newsContents_feedId_fkey; Type: FK CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public."newsContents"
    ADD CONSTRAINT "newsContents_feedId_fkey" FOREIGN KEY ("feedId") REFERENCES "newsFeeds" (id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: newsFeeds newsFeeds_skillId_fkey; Type: FK CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public."newsFeeds"
    ADD CONSTRAINT "newsFeeds_skillId_fkey" FOREIGN KEY ("skillId") REFERENCES skills (id) ON UPDATE CASCADE ON DELETE CASCADE;



--
-- Name: showFeeds showFeeds_skillId_fkey; Type: FK CONSTRAINT; Schema: public; Owner: -
--
ALTER TABLE ONLY public."showFeeds"
    ADD CONSTRAINT "showFeeds_skillId_fkey" FOREIGN KEY ("skillId") REFERENCES skills (id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: oauthApps oauthApps_userId_fkey; Type: FK CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public."oauthApps"
    ADD CONSTRAINT "oauthApps_userId_fkey" FOREIGN KEY ("userId") REFERENCES users (id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: oneTimeShares oneTimeShares_skillId_fkey; Type: FK CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public."oneTimeShares"
    ADD CONSTRAINT "oneTimeShares_skillId_fkey" FOREIGN KEY ("skillId") REFERENCES skills(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: phoneConfirmations phoneConfirmations_skillId_fkey; Type: FK CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public."phoneConfirmations"
    ADD CONSTRAINT "phoneConfirmations_skillId_fkey" FOREIGN KEY ("skillId") REFERENCES skills(id);


--
-- Name: publishedIntents publishedIntents_skillId_fkey; Type: FK CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public."publishedIntents"
    ADD CONSTRAINT "publishedIntents_skillId_fkey" FOREIGN KEY ("skillId") REFERENCES skills(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: publishedMarketDevices publishedMarketDevices_skillId_fkey; Type: FK CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public."publishedMarketDevices"
    ADD CONSTRAINT "publishedMarketDevices_skillId_fkey" FOREIGN KEY ("skillId") REFERENCES skills(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: publishedUserAgreements publishedUserAgreements_skillId_fkey; Type: FK CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public."publishedUserAgreements"
    ADD CONSTRAINT "publishedUserAgreements_skillId_fkey" FOREIGN KEY ("skillId") REFERENCES skills(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: skill_user_share skill_user_share_skill_id_fkey; Type: FK CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.skill_user_share
    ADD CONSTRAINT skill_user_share_skill_id_fkey FOREIGN KEY (skill_id) REFERENCES skills(id);


--
-- Name: skill_user_share skill_user_share_user_id_fkey; Type: FK CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.skill_user_share
    ADD CONSTRAINT skill_user_share_user_id_fkey FOREIGN KEY (user_id) REFERENCES users(id);


--
-- Name: skillsCrypto skillsCrypto_skillId_fkey; Type: FK CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public."skillsCrypto"
    ADD CONSTRAINT "skillsCrypto_skillId_fkey" FOREIGN KEY ("skillId") REFERENCES skills(id);


--
-- Name: skills skills_logoId_fkey; Type: FK CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.skills
    ADD CONSTRAINT "skills_logoId_fkey" FOREIGN KEY ("logoId") REFERENCES images(id) ON UPDATE CASCADE ON DELETE SET NULL;


--
-- Name: skills skills_oauthAppId_fkey; Type: FK CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.skills
    ADD CONSTRAINT "skills_oauthAppId_fkey" FOREIGN KEY ("oauthAppId") REFERENCES "oauthApps"(id);


--
-- Name: skills skills_userId_fkey; Type: FK CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.skills
    ADD CONSTRAINT "skills_userId_fkey" FOREIGN KEY ("userId") REFERENCES users(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: sounds sounds_skillId_fkey; Type: FK CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.sounds
    ADD CONSTRAINT "sounds_skillId_fkey" FOREIGN KEY ("skillId") REFERENCES skills(id);


--
-- Name: subscriptions subscriptions_userId_fkey; Type: FK CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.subscriptions
    ADD CONSTRAINT "subscriptions_userId_fkey" FOREIGN KEY ("userId") REFERENCES users(id);


--
-- Name: userReviews userReviews_skillId_fkey; Type: FK CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public."userReviews"
    ADD CONSTRAINT "userReviews_skillId_fkey" FOREIGN KEY ("skillId") REFERENCES skills(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- PostgreSQL database dump complete
--


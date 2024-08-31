--
-- SQL to create them all the tables.
-- Written in appended form: first part never changes, all new is added in the end in form of a kinda-migration.
-- So when actual migration is needed -- just run statements in the end
-- NB: each migration block should start with line with comment like `Migration #X` -- tests extract this to test migrations
--
-- FIXME: migrate to https://github.com/yandex/pgmigrate ! (pun intended)
--

CREATE TABLE DefaultPaymentInfo
(
    uid    BIGINT PRIMARY KEY,
    cardId VARCHAR(16),
    email  VARCHAR(128)
);


CREATE SEQUENCE PurchaseIdsSeq;

CREATE TABLE UserPurchases
(
    purchaseId     BIGINT PRIMARY KEY,
    uid            BIGINT,
    purchaseToken  VARCHAR(64),
    purchaseDate   TIMESTAMP,
    contentId      VARCHAR(64),
    contentType    VARCHAR(32),
    selectedOption TEXT,
    status         VARCHAR(32),
    callbackDate   TIMESTAMP,
    subscriptionId BIGINT
);

CREATE INDEX UserPurchases_uid_purchaseToken_idx ON UserPurchases (uid, purchaseToken);

CREATE INDEX UserPurchases_status_purchaseDate_part_idx ON UserPurchases (status, purchaseDate) WHERE status IN ('STARTED', 'PROCESSED');


CREATE SEQUENCE PasteIdsSeq;

CREATE TABLE UserPastes
(
    pasteId   BIGINT PRIMARY KEY,
    uid       BIGINT,
    paste     TEXT,
    pasteDate TIMESTAMP
);

CREATE INDEX UserPastes_pasteDate_idx ON UserPastes (pasteDate);

CREATE SEQUENCE SubscriptionProductCodesSeq START WITH 1000;

CREATE TABLE SubscriptionProducts
(
    provider               VARCHAR(32),
    subscriptionPeriodDays SMALLINT,
    trialPeriodDays        SMALLINT,
    price                  NUMERIC(8, 2),
    currency               VARCHAR(8),
    productCode            BIGINT,
    PRIMARY KEY (provider, subscriptionPeriodDays, trialPeriodDays, price, currency)
);


CREATE SEQUENCE SubscriptionIdsSeq;

CREATE TABLE UserSubscriptions
(
    subscriptionId   BIGINT PRIMARY KEY,
    uid              BIGINT,
    subscriptionDate TIMESTAMP,
    contentId        VARCHAR(64),
    contentType      VARCHAR(32),
    selectedOption   TEXT,
    status           VARCHAR(16)
);

-- Migration #1, adding contentItem: see QUASAR-1141
-- NB: when running this make sure to wait for all of the current payments to conclude somehow.
ALTER TABLE UserPurchases
    ADD COLUMN IF NOT EXISTS contentItem TEXT DEFAULT null;

UPDATE UserPurchases
SET contentItem = format('{"contentType": "%s", "id": "%s"}', contentType, contentId)
WHERE contentItem IS NULL;

ALTER TABLE UserSubscriptions
    ADD COLUMN IF NOT EXISTS contentItem TEXT DEFAULT null;

UPDATE UserSubscriptions
SET contentItem = format('{"contentType": "%s", "id": "%s"}', contentType, contentId)
WHERE contentItem IS NULL;

--#2 История покупок и управление подписками - QUASAR-682
CREATE INDEX IF NOT EXISTS UserPurchases_uid_purchaseDate_idx ON UserPurchases (uid, purchaseDate) WHERE status = 'CLEARED';

CREATE INDEX IF NOT EXISTS UserPurchases_uid_purchaseDate_contentOnly_idx ON UserPurchases (uid, purchaseDate) WHERE status = 'CLEARED' AND contentType != 'subscription';

ALTER TABLE UserSubscriptions
    ADD COLUMN IF NOT EXISTS activeTill TIMESTAMP;

--#3 Получение активных подписок и мониторинг подписок зависших в обработке
CREATE INDEX IF NOT EXISTS UserSubscriptions_uid_activeTill_idx ON UserSubscriptions (uid, activeTill);

CREATE INDEX IF NOT EXISTS UserSubscriptions_status_activeTill_part_idx ON UserSubscriptions (status, activeTill) WHERE status IN ('CREATED', 'ACTIVE');

--#4 QUASAR-1528 --Обработка ретраев покупки
ALTER TABLE UserPurchases
    ADD COLUMN IF NOT EXISTS retriesCount INT DEFAULT 0;

-- QUASAR-1580 -- table store purchase locks
CREATE TABLE IF NOT EXISTS UserPurchaseLock
(
    uid        BIGINT,
    provider   VARCHAR(32),
    acquiredAt TIMESTAMP,
    primary key (uid, provider)
);

CREATE TABLE IF NOT EXISTS UserPromoCode
(
    uid                       bigint       not null,
    code                      varchar(128) not null,
    pricingOptionType         varchar(30)  not null,
    subscriptionPeriod        int,
    subscriptionPricingOption text,
    activatedAt               TIMESTAMP,
    primary key (uid, code)
);

--QUASAR-1941
ALTER TABLE UserPurchases
    ADD COLUMN IF NOT EXISTS securityToken VARCHAR(64);
ALTER TABLE UserSubscriptions
    ADD COLUMN IF NOT EXISTS securityToken VARCHAR(64);

--QUASAR-2184
ALTER TABLE UserPromoCode
    ADD COLUMN IF NOT EXISTS provider varchar(32);
ALTER TABLE UserPromoCode
    DROP CONSTRAINT IF EXISTS UserPromoCode_pkey;
ALTER TABLE UserPromoCode
    ADD CONSTRAINT UserPromoCode_uk unique (uid, provider, code);

--QUASAR-1603
ALTER TABLE UserPromoCode
    ADD COLUMN IF NOT EXISTS subscriptionId bigint;

--QUASAR-2231
ALTER TABLE UserPurchases
    ADD COLUMN IF NOT EXISTS provider varchar(32);
ALTER TABLE UserSubscriptions
    ADD COLUMN IF NOT EXISTS provider varchar(32);


--QUASAR-2070
CREATE SEQUENCE IF NOT EXISTS PromoCodeBaseSeq;
CREATE TABLE IF NOT EXISTS PromoCodeBase
(
    id        bigint       not null,
    provider  varchar(32)  not null,
    promoType varchar(128) not null,
    code      varchar(128) not null,
    primary key (id),
    unique (provider, promoType, code)
);

CREATE SEQUENCE IF NOT EXISTS UsedDevicePromoSeq;
CREATE TABLE IF NOT EXISTS UsedDevicePromo
(
    id                  bigint       not null,
    deviceId            varchar(128) not null,
    platform            varchar(256) not null,
    uid                 bigint,
    provider            varchar(256) not null,
    promoActivationTime timestamp,
    codeid              bigint,
    primary key (id),
    constraint UsedDevicePromo_uk_codeid unique (codeid),
    constraint UsedDevicePromo_uk unique (deviceId, platform, provider),
    constraint UsedDevicePromo_fk_codebase foreign key (codeid) references PromoCodeBase (id)
);

CREATE TABLE IF NOT EXISTS QuasarPromoBlacklist
(
    uid varchar(20) not null,
    primary key (uid)
);

--QUASAR-2248
update userpromocode
set provider = subscriptionpricingoption::json ->> 'provider'
where provider is null
  and subscriptionpricingoption::json ->> 'provider' is not null;

update userpromocode
set provider = 'amediateka'
where provider is null
  and subscriptionperiod in (14, 90);

update userpromocode
set provider = 'ivi'
where provider is null
  and subscriptionperiod in (60, 1);

update userpromocode
set provider = 'ivi'
where code in ('fd82ee3cd2e6', '20a61c2e8009');

update userpromocode
set provider = 'amediateka'
where code in ('e6t5e6x2e5vv', 'p2w4y2x9m4jv', 'z7z5p5p3f4jt');

alter table UserPromoCode
    alter column provider set not null;


--QUASAR-2375
ALTER TABLE UserPurchases
    ADD COLUMN IF NOT EXISTS userPrice numeric;
ALTER TABLE UserPurchases
    ADD COLUMN IF NOT EXISTS originalPrice numeric;
ALTER TABLE UserPurchases
    ADD COLUMN IF NOT EXISTS currencyCode varchar(5);

ALTER TABLE UserSubscriptions
    ADD COLUMN IF NOT EXISTS userPrice numeric;
ALTER TABLE UserSubscriptions
    ADD COLUMN IF NOT EXISTS originalPrice numeric;
ALTER TABLE UserSubscriptions
    ADD COLUMN IF NOT EXISTS subscriptionPeriod varchar(20);
ALTER TABLE UserSubscriptions
    ADD COLUMN IF NOT EXISTS trialPeriod varchar(20);
ALTER TABLE UserSubscriptions
    ADD COLUMN IF NOT EXISTS productCode varchar(20);
ALTER TABLE UserSubscriptions
    ADD COLUMN IF NOT EXISTS currencyCode varchar(5);


--QUASAR_2394
CREATE SEQUENCE IF NOT EXISTS skill_info_seq;
CREATE TABLE IF NOT EXISTS skill_info
(
    skill_info_id bigint        not null default nextval('skill_info_seq'),
    skill_uuid    varchar(50)   not null,
    partner_id    bigint        not null,
    private_key   varchar(2048) not null,
    public_key    varchar(1024) not null,
    created_at    timestamp     not null,
    primary key (skill_info_id),
    constraint skill_parameters_uk_skill_id unique (skill_uuid)
);


CREATE SEQUENCE IF NOT EXISTS purchase_offer_seq;
CREATE TABLE IF NOT EXISTS purchase_offer
(
    purchase_offer_id   bigint        not null default nextval('purchase_offer_seq') primary key,
    skill_info_id       bigint        not null references skill_info (skill_info_id),
    uid                 varchar(30)   not null,
    uuid                varchar(40)   not null,
    purchase_request_id varchar(40)   not null,
    title               varchar(1024) not null,
    image_url           varchar(2048),
    description         varchar(2048),
    created_at          timestamp     not null,
    skill_callback_url  varchar(2048) not null,
    pricing_options     TEXT                   DEFAULT null,
    skill_session_id    varchar(64)   not null,
    skill_user_id       varchar(64)   not null,
    skill_name          varchar(2048),
    skill_image_url     varchar(4096),
    status              varchar(16)   not null,
    constraint purchase_offer_uk_partner_ext_id unique (skill_info_id, purchase_request_id),
    constraint purchase_offer_uk_uid_uuid unique (uid, uuid)
);

CREATE SEQUENCE IF NOT EXISTS generic_product_seq;
CREATE TABLE IF NOT EXISTS generic_product
(
    generic_product_id bigint       not null default nextval('generic_product_seq') primary key,
    partner_id         bigint       not null,
    product_code       varchar(256) not null,
    created_at         timestamp    not null,
    constraint generic_product_uk unique (partner_id, product_code)
);
ALTER TABLE UserPurchases
    ADD COLUMN IF NOT EXISTS purchaseOfferId bigint references purchase_offer (purchase_offer_id);

ALTER TABLE UserPurchases
    ADD COLUMN IF NOT EXISTS partnerId bigint;

ALTER TABLE UserPurchases
    ADD COLUMN IF NOT EXISTS paymentProcessor varchar(20);

ALTER TABLE UserSubscriptions
    ADD COLUMN IF NOT EXISTS partnerId bigint;
ALTER TABLE UserSubscriptions
    ADD COLUMN IF NOT EXISTS purchasedContentItem varchar(4000);
ALTER TABLE UserSubscriptions
    ADD COLUMN IF NOT EXISTS paymentProcessor varchar(20);

UPDATE UserPurchases
SET partnerId = 40095249
WHERE provider = 'ivi'
  and partnerId is null;

UPDATE UserPurchases
SET partnerId = 39072166
WHERE provider = 'amediateka'
  and partnerId is null;

UPDATE UserPurchases
SET partnerId = 39564497
WHERE provider = 'litres'
  and partnerId is null;



UPDATE UserSubscriptions
SET partnerId = 40095249
WHERE provider = 'ivi'
  and partnerId is null;

UPDATE UserSubscriptions
SET partnerId = 39072166
WHERE provider = 'amediateka'
  and partnerId is null;

UPDATE UserSubscriptions
SET partnerId = 39564497
WHERE provider = 'litres'
  and partnerId is null;

UPDATE UserSubscriptions
SET purchasedContentItem = selectedOption::json ->> 'purchasingItem'
where purchasedContentItem is null;

/*ALTER TABLE SubscriptionProducts
  ALTER COLUMN partnerId SET NOT NULL;*/

/*ALTER TABLE SubscriptionProducts
  ADD CONSTRAINT SubscriptionProducts_uc unique (partnerId, subscriptionPeriodDays, trialPeriodDays, price, currency);

ALTER TABLE SubscriptionProducts
  DROP CONSTRAINT SubscriptionProducts_pkey;*/


--QUASAR-3157
ALTER TABLE UserPurchases
    ADD COLUMN IF NOT EXISTS refundDate timestamp;

--QUASAR-3013
--CREATE SEQUENCE IF NOT EXISTS skill_merchant_seq;
CREATE TABLE IF NOT EXISTS skill_merchant
(
    --id                  bigint        not null default nextval('skill_merchant_seq') primary key ,
    skill_info_id bigint      not null references skill_info (skill_info_id),
    merchant_key  varchar(50) not null,
    partner_id    bigint      not null,
    constraint skill_merchant_uk unique (skill_info_id, merchant_key),
    constraint skill_merchant_uk2 unique (skill_info_id, partner_id)
);
ALTER TABLE purchase_offer
    ADD COLUMN IF NOT EXISTS partner_id BIGINT;
ALTER TABLE skill_info
    ALTER COLUMN partner_id DROP NOT NULL;

ALTER TABLE purchase_offer
    ADD COLUMN IF NOT EXISTS delivery_info text;

ALTER TABLE purchase_offer
    ALTER COLUMN uid DROP NOT NULL;


--QUASAR-
ALTER TABLE purchase_offer
    ADD COLUMN IF NOT EXISTS merchant_key varchar(64);
ALTER TABLE UserPurchaseLock
    ALTER COLUMN provider type varchar(128);

--QUASAR-3520: Добавить проверку, что платеж действительно был заклирен, после /clear
ALTER TABLE UserPurchases
    ADD COLUMN IF NOT EXISTS clearDate timestamp;

--QUASAR-3610
ALTER TABLE UserPurchases
    ADD COLUMN IF NOT EXISTS merchantId BIGINT;
ALTER TABLE skill_merchant
    RENAME COLUMN partner_id TO service_merchant_id;
ALTER TABLE skill_merchant
    ADD COLUMN IF NOT EXISTS entity_id VARCHAR(128) NOT NULL;
ALTER TABLE skill_merchant
    ADD COLUMN IF NOT EXISTS description VARCHAR(4000);
ALTER TABLE skill_info
    ADD COLUMN IF NOT EXISTS owner_uid BIGINT NOT NULL;
ALTER TABLE purchase_offer
    ADD COLUMN IF NOT EXISTS initial_device_id varchar(64);

--QUASAR-4826
ALTER TABLE PromoCodeBase
    ADD COLUMN IF NOT EXISTS task_id varchar(64);


--PASKILLS-4208
ALTER TABLE PromoCodeBase
    ADD COLUMN IF NOT EXISTS platform varchar(256);

-- look lower
-- create or replace view v_used_device_promo as
-- select dp.id,
--        dp.deviceid,
--        dp.platform,
--        dp.uid,
--        dp.provider,
--        dp.promoactivationtime,
--        dp.codeid,
--        p.promotype,
--        p.code,
--        p.task_id,
--        p.platform as core_platform
-- from useddevicepromo dp
--          left join promocodebase p on dp.codeid = p.id;

--SPI-9195
create index if not exists promocodebase_idx1 on PromoCodeBase (provider, promoType, code) where platform is null;
create index if not exists promocodebase_idx2 on PromoCodeBase (provider, promoType, platform, code);

--PASKILLS-5025
create or replace view v_used_device_promo_stats as
select p.promotype,
       p.provider,
       p.platform,
       p.task_id,
       count(d.codeid)                                        used_count,
       count(p.id)                                            total_count,
       count(p.id) - count(d.codeid)                          left_count,
       100 * (count(p.id) - count(d.codeid)) / count(p.id) as left_pcnt
from promocodebase p
         left join useddevicepromo d on p.id = d.codeid
group by p.promotype, p.platform, p.task_id, p.provider;

-- disable as it failed on ut
--grant select on v_used_device_promo_stats to quasar_ro;

--PASKILLS-5304

CREATE TABLE IF NOT EXISTS skill_product
(
    uuid       uuid          not null,
    skill_uuid varchar(50)   not null references skill_info (skill_uuid),
    name       varchar(512)  not null,
    type       varchar(64)   not null,
    price      numeric(8, 2) not null,
    deleted    boolean       not null,
    constraint skill_products_uk primary key (skill_uuid, uuid)
);

CREATE SEQUENCE IF NOT EXISTS product_token_seq;

CREATE TABLE IF NOT EXISTS product_token
(
    id           bigint       not null default nextval('product_token_seq') primary key,
    product_uuid uuid         not null,
    skill_uuid   varchar(50)  not null,
    provider     varchar(256) not null,
    code         varchar(64)  not null,
    reusable     boolean      not null,
    foreign key (product_uuid, skill_uuid) references skill_product (uuid, skill_uuid),
    constraint products_tokens_uk unique (skill_uuid, code)
);

CREATE SEQUENCE IF NOT EXISTS user_skill_product_seq;

CREATE TABLE IF NOT EXISTS user_skill_product
(
    id           bigint      not null default nextval('user_skill_product_seq') primary key,
    uid          varchar(30) not null,
    token_id     bigint references product_token (id),
    product_uuid uuid        not null,
    skill_uuid   varchar(50) not null,
    foreign key (product_uuid, skill_uuid) references skill_product (uuid, skill_uuid),
    constraint user_skill_product_uk unique (uid, product_uuid, skill_uuid)
);

CREATE INDEX IF NOT EXISTS user_skill_product_idx on user_skill_product (uid);

-- PASKILLS-5845
alter table UserPurchases
    add column if not exists skillInfoId bigint default null references skill_info (skill_info_id);

alter table UserPurchases
    add column if not exists merchantName varchar(2048) default null;

alter table skill_info
    add column if not exists slug varchar(2048) default null;

--PASKILLS-5153--
alter table purchase_offer
    add column if not exists webhook_request text;

--PASKILLS--
alter table UserPurchases
    add column if not exists userSkillProductId bigint
        references user_skill_product (id) default null;

alter table user_skill_product
    add column if not exists skill_name varchar(2048) default null;
alter table user_skill_product
    add column if not exists skill_image_url varchar(4096) default null;

--PASKILLS-6785--
alter table purchase_offer
    add column if not exists test_payment boolean default false;


--PASKILLS
create table if not exists promocode_prototype
(
    promocode_prototype_id serial primary key,
    promo_type             varchar(128) not null,
    platform               varchar(128) not null,
    code                   varchar(128) not null,
    task_id                varchar(128) not null,
    constraint promocode_prototype_uk unique (promo_type, platform)
);


ALTER TABLE PromoCodeBase
    ADD COLUMN IF NOT EXISTS prototype_id int default null references promocode_prototype (promocode_prototype_id);

create or replace view v_used_device_promo as
select dp.id,
       dp.deviceid,
       dp.platform,
       dp.uid,
       dp.provider,
       dp.promoactivationtime,
       dp.codeid,
       p.promotype,
       p.code,
       p.task_id,
       p.platform as core_platform,
       pp.promocode_prototype_id,
       pp.task_id as prototype_task_id
from useddevicepromo dp
         left join promocodebase p on dp.codeid = p.id
         left join promocode_prototype pp on p.prototype_id = pp.promocode_prototype_id;

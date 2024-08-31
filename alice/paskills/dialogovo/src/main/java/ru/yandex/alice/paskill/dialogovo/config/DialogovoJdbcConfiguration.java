package ru.yandex.alice.paskill.dialogovo.config;

import java.sql.SQLException;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

import javax.annotation.Nonnull;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.core.type.TypeReference;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.jetbrains.annotations.NotNull;
import org.postgresql.jdbc.PgArray;
import org.postgresql.util.PGobject;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.core.convert.converter.Converter;

import ru.yandex.alice.kronstadt.core.db.PgConverter;
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillInfoDB;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.providers.NewsSkillInfoDB;

@Configuration
public class DialogovoJdbcConfiguration {

    private final ObjectMapper objectMapper;

    public DialogovoJdbcConfiguration(ObjectMapper objectMapper) {
        this.objectMapper = objectMapper;
    }

    @Bean
    PgConverter<SkillInfoDB.BackendSettings, String> converter1() {
        return new PgConverter<SkillInfoDB.BackendSettings, String>(objectMapper) {
            @Override
            public String convert(SkillInfoDB.BackendSettings source) {
                return jsonToString(source);
            }
        };
    }

    @Bean
    PgConverter<String, SkillInfoDB.BackendSettings> converter2() {
        return new PgConverter<String, SkillInfoDB.BackendSettings>(objectMapper) {
            @Override
            public SkillInfoDB.BackendSettings convert(String source) {
                return stringToJsonObject(source, SkillInfoDB.BackendSettings.class);
            }
        };
    }

    @Bean
    PgObjectConverter<SkillInfoDB.BackendSettings, PGobject> converter3() {
        return new PgObjectConverter<SkillInfoDB.BackendSettings, PGobject>(objectMapper) {
            @Override
            public PGobject convert(SkillInfoDB.BackendSettings source) {
                return toPgJson(source);
            }
        };

    }

    @Bean
    PgConverter<PGobject, SkillInfoDB.BackendSettings> converter4() {
        return new PgConverter<PGobject, SkillInfoDB.BackendSettings>(objectMapper) {
            @Override
            public SkillInfoDB.BackendSettings convert(PGobject source) {
                return stringToJsonObject(source.getValue(),
                        SkillInfoDB.BackendSettings.class);
            }
        };

    }

    @Bean
    PgConverter<NewsSkillInfoDB.BackendSettings, String> converter5() {
        return new PgConverter<NewsSkillInfoDB.BackendSettings, String>(objectMapper) {
            @Override
            public String convert(NewsSkillInfoDB.BackendSettings source) {
                return jsonToString(source);
            }
        };

    }

    @Bean
    PgConverter<String, NewsSkillInfoDB.BackendSettings> converter6() {
        return new PgConverter<String, NewsSkillInfoDB.BackendSettings>(objectMapper) {
            @Override
            public NewsSkillInfoDB.BackendSettings convert(String source) {
                return stringToJsonObject(source, NewsSkillInfoDB.BackendSettings.class);
            }
        };

    }

    @Bean
    PgObjectConverter<NewsSkillInfoDB.BackendSettings, PGobject> converter7() {
        return new PgObjectConverter<NewsSkillInfoDB.BackendSettings, PGobject>(objectMapper) {
            @Override
            public PGobject convert(NewsSkillInfoDB.BackendSettings source) {
                return toPgJson(source);
            }
        };

    }

    @Bean
    PgConverter<PGobject, NewsSkillInfoDB.BackendSettings> converter8() {
        return new PgConverter<PGobject, NewsSkillInfoDB.BackendSettings>(objectMapper) {
            @Override
            public NewsSkillInfoDB.BackendSettings convert(PGobject source) {
                return stringToJsonObject(source.getValue(),
                        NewsSkillInfoDB.BackendSettings.class);
            }
        };

    }

    @Bean
    PgConverter<SkillInfoDB.PublishingSettings, String> converter9() {
        return new PgConverter<SkillInfoDB.PublishingSettings, String>(objectMapper) {
            @Override
            public String convert(SkillInfoDB.PublishingSettings source) {
                return jsonToString(source);
            }
        };

    }

    @Bean
    PgConverter<String, SkillInfoDB.PublishingSettings> converter10() {
        return new PgConverter<String, SkillInfoDB.PublishingSettings>(objectMapper) {
            @Override
            public SkillInfoDB.PublishingSettings convert(String source) {
                return stringToJsonObject(source, SkillInfoDB.PublishingSettings.class);
            }
        };

    }

    @Bean
    PgObjectConverter<SkillInfoDB.PublishingSettings, PGobject> converter11() {
        return new PgObjectConverter<SkillInfoDB.PublishingSettings, PGobject>(objectMapper) {
            @Override
            public PGobject convert(SkillInfoDB.PublishingSettings source) {
                return toPgJson(source);
            }
        };

    }

    @Bean
    PgConverter<PGobject, SkillInfoDB.PublishingSettings> converter12() {
        return new PgConverter<PGobject, SkillInfoDB.PublishingSettings>(objectMapper) {
            @Override
            public SkillInfoDB.PublishingSettings convert(PGobject source) {
                return stringToJsonObject(source.getValue(),
                        SkillInfoDB.PublishingSettings.class);
            }
        };

    }

    @Bean
    Converter<PGobject, SkillInfoDB.UserFeatures> converter13() {
        return new Converter<PGobject, SkillInfoDB.UserFeatures>() {
            private final TypeReference<Map<String, ?>> ref = new TypeReference<>() {
            };

            @Override
            public SkillInfoDB.UserFeatures convert(PGobject source) {
                try {
                    Map<String, ?> val = DialogovoJdbcConfiguration.this.objectMapper.readValue(source.getValue(), ref);
                    return new SkillInfoDB.UserFeatures(val);
                } catch (JsonProcessingException e) {
                    throw new RuntimeException();
                }
            }
        };
    }

    @Bean
    PgConverter<String, SkillInfoDB.UserFeatures> converter14() {
        return new PgConverter<String, SkillInfoDB.UserFeatures>(objectMapper) {
            @Override
            public SkillInfoDB.UserFeatures convert(String source) {
                return stringToJsonObject(source, SkillInfoDB.UserFeatures.class);
            }
        };
    }

    @Bean
    PgObjectConverter<SkillInfoDB.UserFeatures, PGobject> converter15() {
        return new PgObjectConverter<SkillInfoDB.UserFeatures, PGobject>(objectMapper) {
            @Override
            public PGobject convert(SkillInfoDB.UserFeatures source) {
                return toPgJson(source);
            }
        };
    }

    @Bean
    PgConverter<SkillInfoDB.UserFeatures, String> converter16() {
        return new PgConverter<SkillInfoDB.UserFeatures, String>(objectMapper) {
            @Override
            public String convert(SkillInfoDB.UserFeatures source) {
                return jsonToString(source);
            }
        };

    }

    @Bean
    PgConverter<PGobject, NewsSkillInfoDB.UserFeatures> converter17() {
        return new PgConverter<PGobject, NewsSkillInfoDB.UserFeatures>(objectMapper) {
            private final TypeReference<Map<String, ?>> ref = new TypeReference<>() {
            };

            @Override
            public NewsSkillInfoDB.UserFeatures convert(PGobject source) {
                try {
                    Map<String, ?> val = DialogovoJdbcConfiguration.this.objectMapper.readValue(source.getValue(), ref);
                    return new NewsSkillInfoDB.UserFeatures(val);
                } catch (JsonProcessingException e) {
                    throw new RuntimeException();
                }
            }
        };
    }

    @Bean
    PgConverter<String, NewsSkillInfoDB.UserFeatures> converter18() {
        return new PgConverter<String, NewsSkillInfoDB.UserFeatures>(objectMapper) {
            @Override
            public NewsSkillInfoDB.UserFeatures convert(String source) {
                return stringToJsonObject(source, NewsSkillInfoDB.UserFeatures.class);
            }
        };
    }

    @Bean
    PgObjectConverter<NewsSkillInfoDB.UserFeatures, PGobject> converter19() {
        return new PgObjectConverter<NewsSkillInfoDB.UserFeatures, PGobject>(objectMapper) {
            @Override
            public PGobject convert(NewsSkillInfoDB.UserFeatures source) {
                return toPgJson(source);
            }
        };
    }

    @Bean
    PgConverter<NewsSkillInfoDB.UserFeatures, String> converter20() {
        return new PgConverter<NewsSkillInfoDB.UserFeatures, String>(objectMapper) {
            @Override
            public String convert(NewsSkillInfoDB.UserFeatures source) {
                return jsonToString(source);
            }
        };
    }

    @Bean
    Converter<PgArray, List<String>> pgArrayToListStringConverterBean() {
        return pgArrayToListStringConverter();
    }

    @Nonnull
    private Converter<PgArray, List<String>> pgArrayToListStringConverter() {
        return new Converter<PgArray, List<String>>() {
            @Override
            public List<String> convert(PgArray source) {
                try {
                    final Object[] array = (Object[]) source.getArray();
                    if (array != null && array.length > 0) {
                        var result = new ArrayList<String>(array.length);
                        for (Object o : array) {
                            if (o instanceof String) {
                                result.add((String) o);
                            } else if (o instanceof PGobject) {
                                // for enums
                                var pgo = (PGobject) o;
                                result.add(pgo.getValue());
                            } else {
                                throw new IllegalArgumentException("cant convert PgArray to List<String> - " +
                                        o.getClass());
                            }
                        }
                        return result;
                    } else {
                        return new ArrayList<>();
                    }
                } catch (SQLException e) {
                    throw new RuntimeException(e);
                }
            }
        };
    }

    private abstract static class PgObjectConverter<From, To> extends PgConverter<From, To> {
        PgObjectConverter(@NotNull ObjectMapper objectMapper) {
            super(objectMapper);
        }

        @Nonnull
        protected PGobject toPgJson(Object source) {
            PGobject obj = new PGobject();
            obj.setType("json");
            try {
                obj.setValue(jsonToString(source));
            } catch (SQLException e) {
                throw new RuntimeException(e);
            }
            return obj;
        }
    }


}

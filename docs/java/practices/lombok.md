— Что и зачем?
 
— Java многословна, использование Lombok'а позволяет избавиться от большого количества boilerplate-кода,
а также избежать некоторых видов ошибок (например, нет необходимости добавлять новое поле класса в
`toString/equals/hashCode`).

В двух словах:
1) Lombok **разрешен** в Аркадии для прикладного кода, Lombok **запрещено** использовать при написании общих библиотек.
2) Решение по использования/неиспользованию Lombok'a в проекте принимает каждая команда **самостоятельно**.
3) **Не рекомендуется** использовать аннотации: `@SneakyThrows`, `@Cleanup`, `@val`, `@var`. 
С осторожностью следует использовать аннотации из пакета `lombok.experimental`.
4) Из коробки идёт delombok, позволяющий при необходимости избавиться от аннотаций Lombok'a, сгенерировав эквивалентный pure java-код.
5) Пример подключения к проекту в Аркадии: https://a.yandex-team.ru/review/1313122/files#file-0-38430949.

### Основные аннотации
#### `ToString`
[Официальная документация](https://projectlombok.org/features/ToString)

Генерит переопределение метода `Object.toString`. По умолчанию использует геттеры (параметр `doNotUseGetters`),
добавляет в результат название полей (параметр `includeFieldsName`) и не использует метод `toString` родительского класса 
(параметр `callSuper`). Не рекомендуется использовать параметры `of` и `exclude`, так как они будут скоро помечены как 
`@Deprecated`, вместо них стоит использовать `@ToString.Exclude` для явной пометки всех полей, которые не должны 
использоваться в `toString`. Важно не забывать помечать `@ToString.Exclude` поля, которые создают циклическую 
зависимость между классами, Lombok никак это не отслеживает и при вызове `toString` будет бесконечная рекурсия 
со `StackOverflowException` в итоге.
 
В большинстве случаев лучше использовать не напрямую, а через `@Value` или `@Data`.

{% cut "Пример" %}
```java
@ToString
public class A {

    @ToString.Exclude
    private int x;

    private List<Long> ys;

    private String z;
}

// delombok
public class A {

    private int x;

    private List<Long> ys;

    private String z;

    public String toString() {
        return "A(ys=" + this.ys + ", z=" + this.z + ")";
    }
}
```
{% endcut %}

#### `@EqualsAndHashCode`
[Официальная документация](https://projectlombok.org/features/EqualsAndHashCode)

Генерит переопределение методов `Object.equals` и `Object.hashCode`. Во многом аннотация похожа на `@ToString`.
По умолчанию использует геттеры (параметр `doNotUseGetters`) и не использует методы `equals/hashCode` родительского 
класса (параметр `callSuper`). Не рекомендуется использовать параметры `of` и `exclude`, так как они будут скоро
помечены как `@Deprecated`, вместо них стоит использовать `@EqualsAndHashCode.Exclude` для явной пометки всех полей,
которые не должны использоваться в `equals/hashCode`. Важно не забывать помечать `@EqualsAndHashCode.Exclude` поля,
которые создают циклическую зависимость между классами, Lombok никак это не отслеживает и при вызове
`equals/hachCode` будет бесконечная рекурсия со `StackOverflowException` в итоге.

В большинстве случаев лучше использовать не напрямую, а через `@Value` или `@Data`.

{% cut "Пример" %}
```java
@EqualsAndHashCode
public class A {

    @EqualsAndHashCode.Exclude
    private int x;

    private List<Long> ys;

    private String z;
}

// delombok
public class A {

    private int x;

    private List<Long> ys;

    private String z;

    public boolean equals(final Object o) {
        if (o == this) {
            return true;
        }
        if (!(o instanceof A)) {
            return false;
        }
        final A other = (A) o;
        if (!other.canEqual((Object) this)) {
            return false;
        }
        final Object this$ys = this.ys;
        final Object other$ys = other.ys;
        if (this$ys == null ? other$ys != null : !this$ys.equals(other$ys)) {
            return false;
        }
        final Object this$z = this.z;
        final Object other$z = other.z;
        if (this$z == null ? other$z != null : !this$z.equals(other$z)) {
            return false;
        }
        return true;
    }

    protected boolean canEqual(final Object other) {
        return other instanceof A;
    }

    public int hashCode() {
        final int PRIME = 59;
        int result = 1;
        final Object $ys = this.ys;
        result = result * PRIME + ($ys == null ? 43 : $ys.hashCode());
        final Object $z = this.z;
        result = result * PRIME + ($z == null ? 43 : $z.hashCode());
        return result;
    }
}
```
{% endcut %}

#### `@Getter` и `@Setter`
[Официальная документация](https://projectlombok.org/features/GetterSetter)

Будучи навешанными на класс эти аннотации генерируют методы `getX`для всех полей объекта и методы `setX` для всех
полей объекта не помеченных модификатором `final`.

В большинстве случаев лучше использовать не напрямую, а через `@Value` или `@Data`.

{% cut "Пример" %}
```java
@Getter
@Setter
public class A {

    private int x;

    private List<Long> ys = null;

    private String z;
}

// delombok
public class A {

    private int x;

    private final List<Long> ys = null;

    private String z;

    public int getX() {
        return this.x;
    }

    public List<Long> getYs() {
        return this.ys;
    }

    public String getZ() {
        return this.z;
    }

    public void setX(int x) {
        this.x = x;
    }

    public void setZ(String z) {
        this.z = z;
    }
}
```
{% endcut %}

#### Конструкторы
[Официальная документация](https://projectlombok.org/features/constructor)

1) `@NoArgsConstuctor` — генерирует конструктор без параметров.
2) `@AllArgsConstructor` — генерирует конструктор с параметрами, соответствующими всем полям класса.
3) `@RequiredArgsConstructor` — генерирует конструктор с параметрами для полей, помеченных модификатором `final`.

Важно: при наследовании работает только в том случае, если у базового класса есть конструктор без параметров.
Возможность указать конкретный конструктор базового класса отсутствует.
В этом случае лучше явно сгенерировать конструктор(ы), а не полагаться на lombok.

Для каждого конструктора можно задать модификатор видимости (параметр `access`) и задать имя статического метода,
вызывающего приватный конструктор (параметр `staticMethod`).

{% cut "Пример" %}
```java
@RequiredArgsConstructor
@AllArgsConstructor(access = AccessLevel.PACKAGE)
public class A {

    private int x;

    private final List<Long> ys;

    private String z;
}

// delombok
public class A {

    private int x;

    private final List<Long> ys;

    private String z;

    public A(List<Long> ys) {
        this.ys = ys;
    }

    A(int x, List<Long> ys, String z) {
        this.x = x;
        this.ys = ys;
        this.z = z;
    }
}
```
{% endcut %}

#### `@Data`
[Официальная документация](https://projectlombok.org/features/Data)

Одна из основных аннотаций lombok'а, включает в себя следующие аннотации:
- `@Getter`
- `@Setter`
- `@RequiredArgsConstructor`
- `@ToString`
- `@EqualsAndHashCode`

Генерирует всю необходимую boilerplate-обёртку для конкретного класса. Вместо конструктора с обязательными параметрами можно задать статический конструктор (параметр `staticConstructor`).

{% cut "Пример" %}
```java
@Data
public class A {

    private int x;

    private final List<Long> ys;

    private String z;
}

// delombok
public class A {

    private int x;

    private final List<Long> ys;

    private String z;

    public A(List<Long> ys) {
        this.ys = ys;
    }

    public int getX() {
        return this.x;
    }

    public List<Long> getYs() {
        return this.ys;
    }

    public String getZ() {
        return this.z;
    }

    public void setX(int x) {
        this.x = x;
    }

    public void setZ(String z) {
        this.z = z;
    }

    public boolean equals(final Object o) {
        if (o == this) {
            return true;
        }
        if (!(o instanceof A)) {
            return false;
        }
        final A other = (A) o;
        if (!other.canEqual((Object) this)) {
            return false;
        }
        if (this.getX() != other.getX()) {
            return false;
        }
        final Object this$ys = this.getYs();
        final Object other$ys = other.getYs();
        if (this$ys == null ? other$ys != null : !this$ys.equals(other$ys)) {
            return false;
        }
        final Object this$z = this.getZ();
        final Object other$z = other.getZ();
        if (this$z == null ? other$z != null : !this$z.equals(other$z)) {
            return false;
        }
        return true;
    }

    protected boolean canEqual(final Object other) {
        return other instanceof A;
    }

    public int hashCode() {
        final int PRIME = 59;
        int result = 1;
        result = result * PRIME + this.getX();
        final Object $ys = this.getYs();
        result = result * PRIME + ($ys == null ? 43 : $ys.hashCode());
        final Object $z = this.getZ();
        result = result * PRIME + ($z == null ? 43 : $z.hashCode());
        return result;
    }

    public String toString() {
        return "A(x=" + this.getX() + ", ys=" + this.getYs() + ", z=" + this.getZ() + ")";
    }
}
```
{% endcut %}

#### `@Value`
[Официальная документация](https://projectlombok.org/features/Value)

Одна из основных аннотаций lombok'а, включает в себя следующие вещи:
- `@Getter`
- `@AllArgsConstructor`
- `@ToString`
- `@EqualsAndHashCode`
- `@FieldDefaults(makeFinal=true, level=AccessLevel.PRIVATE)`
- помечает сам класс модификатором `final`

Задаёт иммутабельный класс. Так же как и при использовании `@Data` вместо конструктора с обязательными параметрами 
можно задать статический конструктор (параметр `staticConstructor`).
 
Важно: полученный класс не является полностью иммутабельным, так как поля могут быть мутабельными и их всё ещё
можно изменить через геттеры, lombok не делает здесь никаких обёрток/проверок.
Подробнее про иммутабельность смотрите в описании `@Singular`.

{% cut "Пример" %}
```java
@Value
public class A {

    private int x;

    private final List<Long> ys;

    private String z;
}

// delombok
public final class A {

    private final int x;

    private final List<Long> ys;

    private final String z;

    public A(int x, List<Long> ys, String z) {
        this.x = x;
        this.ys = ys;
        this.z = z;
    }

    public int getX() {
        return this.x;
    }

    public List<Long> getYs() {
        return this.ys;
    }

    public String getZ() {
        return this.z;
    }

    public boolean equals(final Object o) {
        if (o == this) {
            return true;
        }
        if (!(o instanceof A)) {
            return false;
        }
        final A other = (A) o;
        if (this.getX() != other.getX()) {
            return false;
        }
        final Object this$ys = this.getYs();
        final Object other$ys = other.getYs();
        if (this$ys == null ? other$ys != null : !this$ys.equals(other$ys)) {
            return false;
        }
        final Object this$z = this.getZ();
        final Object other$z = other.getZ();
        if (this$z == null ? other$z != null : !this$z.equals(other$z)) {
            return false;
        }
        return true;
    }

    public int hashCode() {
        final int PRIME = 59;
        int result = 1;
        result = result * PRIME + this.getX();
        final Object $ys = this.getYs();
        result = result * PRIME + ($ys == null ? 43 : $ys.hashCode());
        final Object $z = this.getZ();
        result = result * PRIME + ($z == null ? 43 : $z.hashCode());
        return result;
    }

    public String toString() {
        return "A(x=" + this.getX() + ", ys=" + this.getYs() + ", z=" + this.getZ() + ")";
    }
}
```
{% endcut %}

#### `@Builder`
[Официальная документация](https://projectlombok.org/features/Builder)

Генерирует приватный класс-билдер для аннотируемого класса. Помимо самого класса можно вешать также на статические
методы и на конструкторы. По умолчанию создаёт приватный конструктор со всеми полями и использует его. Можно задавать
имя статического метода для создания билдера (параметр `builderMethodName`), 
имя метода для создания (параметр `buildMethodName`) и имя класса-билдера (параметр `builderClassName`).

При выставлении параметра `toBuilder` в `true` генерируется метод объекта `toBuilder`, возвращающий билдер для
создания идентичного объекта с уже проставленными полями.

{% cut "Пример" %}
```java
@Value
@Builder(toBuilder = true)
public class A {

    private int x;

    private final List<Long> ys;

    private String z;
}

// delombok
@Value
public static final class A {

    private final int x;

    private final List<Long> ys;

    private final String z;

    A(int x, List<Long> ys, String z) {
        this.x = x;
        this.ys = ys;
        this.z = z;
    }

    public static ABuilder builder() {
        return new ABuilder();
    }

    public ABuilder toBuilder() {
        return new ABuilder().x(this.x).ys(this.ys).z(this.z);
    }

    public static class ABuilder {
        private int x;
        private List<Long> ys;
        private String z;

        ABuilder() {
        }

        public ABuilder x(int x) {
            this.x = x;
            return this;
        }

        public ABuilder ys(List<Long> ys) {
            this.ys = ys;
            return this;
        }

        public ABuilder z(String z) {
            this.z = z;
            return this;
        }

        public A build() {
            return new A(x, ys, z);
        }

        public String toString() {
            return "Z.A.ABuilder(x=" + this.x + ", ys=" + this.ys + ", z=" + this.z + ")";
        }
    }
}
```
{% endcut %}

#### `@Singular`
Используется для аннотирования полей-коллекций вместе с аннотацией `@Builder`. Даёт две полезные вещи:
- генерирует в билдере для полей-коллекций два метода: `ys(Collection<T> ys)` и `y(T y)`. Второй метод позволяет добавлять данные коллекции по одному.
- будет использовать иммутабельные обёртки коллекций.

{% cut "Пример" %}
```
@Value
public class A {
    private final long x;
    @Singular
    private final List<Long> ys;
}

// delombok

@Value
public class A {
    private final long x;
    private final List<Long> ys;

    A(long x, List<Long> ys) {
        this.x = x;
        this.ys = ys;
    }

    public static ABuilder builder() {
        return new ABuilder();
    }

    public static class ABuilder {
        private long x;
        private ArrayList<Long> ys;

        ABuilder() {
        }

        public ABuilder x(long x) {
            this.x = x;
            return this;
        }

        public ABuilder y(Long y) {
            if (this.ys == null) {
                this.ys = new ArrayList<Long>();
            }
            this.ys.add(y);
            return this;
        }

        public ABuilder ys(Collection<? extends Long> ys) {
            if (this.ys == null) {
                this.ys = new ArrayList<Long>();
            }
            this.ys.addAll(ys);
            return this;
        }

        public ABuilder clearYs() {
            if (this.ys != null) {
                this.ys.clear();
            }
            return this;
        }

        public A build() {
            List<Long> ys;
            switch (this.ys == null ? 0 : this.ys.size()) {
                case 0:
                    ys = java.util.Collections.emptyList();
                    break;
                case 1:
                    ys = java.util.Collections.singletonList(this.ys.get(0));
                    break;
                default:
                    ys = java.util.Collections.unmodifiableList(new ArrayList<Long>(this.ys));
            }

            return new A(x, ys);
        }

        public String toString() {
            return "PriceYtToPostgresConvertTest.A.ABuilder(x=" + this.x + ", ys=" + this.ys + ")";
        }
    }
}
```
{% endcut %}

#### `lombok.@NonNull`

**Не рекомендуется к использованию, вместо неё лучше использовать `javax.annotation.@Nonnull`**,
с которой ломбок работает аналогичным образом.

При любой из этих двух аннотаций будет добавлена автоматическая проверка на `null` при присваивании поля.
Позволяет быстро отловить забытое в билдере поле. Под капотом используется что-то наподобие
`Objects.requireNonNull(object)` при присваивании.

#### `@With`
[Официальная документация](https://projectlombok.org/features/With)

Генерирует метод `withFieldName` для конкретного поля либо для всех полей, если аннотирован класс.
Для работы необходим конструктор (допускается приватный) со всеми параметрами, отлично дополняет аннотацию `@Value`.

{% cut "Пример" %}
```java
@Wither
@AllArgsConstructor
public class A {

    private int x;

    private final List<Long> ys;

    private String z;
}

// delombok
@AllArgsConstructor
public class A {

    private int x;

    private final List<Long> ys;

    private String z;

    public A withX(int x) {
        return this.x == x ? this : new A(x, this.ys, this.z);
    }

    public A withYs(List<Long> ys) {
        return this.ys == ys ? this : new A(this.x, ys, this.z);
    }

    public A withZ(String z) {
        return this.z == z ? this : new A(this.x, this.ys, z);
    }
}
```
{% endcut %}

#### `@Slf4j`
[Официальная документация](https://projectlombok.org/features/log)

Генерирует slf4j-логгер для аннотируемого класса, частный случай аннотации `Log`.

{% cut "Пример" %}
```
@Slf4j
public class S {
}

// delombok
public class S {
    private static final Logger log = org.slf4j.LoggerFactory.getLogger(S.class);
}
```
{% endcut %}

### Наследование (`@SuperBuilder`)

Lombok плохо работает с наследованием: конструкторы придётся создавать руками, билдеры тоже.
Разрешено использовать экспериментальную аннотацию Lombok'а `@SuperBuilder`.

[Официальная документация](https://projectlombok.org/features/experimental/SuperBuilder)

В отличие от `@Builder` это аннотация может быть навешена только на сам класс. Также требуется, чтобы эта аннотация
была на всех классах-предках.
 
Важно не забывать явно прописывать у классов-наследников `@EqualsAndHashCode(callSuper = true)` и
`@ToString(callSuper = true)`.

{% cut "Пример" %}
```java
@Data
@SuperBuilder
public static class AType {
    private final long x;
}


@Data
@ToString(callSuper = true)
@EqualsAndHashCode(callSuper = true)
@SuperBuilder
public static class BType extends AType {
    private final String z;
}

@Value
@ToString(callSuper = true)
@EqualsAndHashCode(callSuper = true)
@SuperBuilder
public static class CType extends BType {
    private final List<Long> ys;
}

// delombok
public class AType {
    private final long x;

    protected AType(ATypeBuilder<?, ?> b) {
        this.x = b.x;
    }

    public static ATypeBuilder<?, ?> builder() {
        return new ATypeBuilderImpl();
    }

    public long getX() {
        return this.x;
    }

    public static abstract class ATypeBuilder<C extends AType, B extends AType.ATypeBuilder<C, B>> {
        private long x;

        public B x(long x) {
            this.x = x;
            return self();
        }

        protected abstract B self();

        public abstract C build();

        public String toString() {
            return "PriceYtToPostgresConvertTest.AType.ATypeBuilder(x=" + this.x + ")";
        }
    }

    private static final class ATypeBuilderImpl extends ATypeBuilder<AType, ATypeBuilderImpl> {
        private ATypeBuilderImpl() {
        }

        protected AType.ATypeBuilderImpl self() {
            return this;
        }

        public AType build() {
            return new AType(this);
        }
    }
}


public class BType extends AType {
    private final String z;

    protected BType(BTypeBuilder<?, ?> b) {
        super(b);
        this.z = b.z;
    }

    public static BTypeBuilder<?, ?> builder() {
        return new BTypeBuilderImpl();
    }

    public String getZ() {
        return this.z;
    }

    public String toString() {
        return "PriceYtToPostgresConvertTest.BType(super=" + super.toString() + ", z=" + this.getZ() + ")";
    }

    public static abstract class BTypeBuilder<C extends BType, B extends BType.BTypeBuilder<C, B>> extends ATypeBuilder<C, B> {
        private String z;

        public B z(String z) {
            this.z = z;
            return self();
        }

        protected abstract B self();

        public abstract C build();

        public String toString() {
            return "PriceYtToPostgresConvertTest.BType.BTypeBuilder(super=" + super.toString() + ", z=" + this.z + ")";
        }
    }

    private static final class BTypeBuilderImpl extends BTypeBuilder<BType, BTypeBuilderImpl> {
        private BTypeBuilderImpl() {
        }

        protected BType.BTypeBuilderImpl self() {
            return this;
        }

        public BType build() {
            return new BType(this);
        }
    }
}

public final class CType extends BType {
    private final List<Long> ys;

    protected CType(CTypeBuilder<?, ?> b) {
        super(b);
        this.ys = b.ys;
    }

    public static CTypeBuilder<?, ?> builder() {
        return new CTypeBuilderImpl();
    }

    public List<Long> getYs() {
        return this.ys;
    }

    public static abstract class CTypeBuilder<C extends CType, B extends CType.CTypeBuilder<C, B>> extends BTypeBuilder<C, B> {
        private List<Long> ys;

        public B ys(List<Long> ys) {
            this.ys = ys;
            return self();
        }

        protected abstract B self();

        public abstract C build();

        public String toString() {
            return "PriceYtToPostgresConvertTest.CType.CTypeBuilder(super=" + super.toString() + ", ys=" + this.ys + ")";
        }
    }

    private static final class CTypeBuilderImpl extends CTypeBuilder<CType, CTypeBuilderImpl> {
        private CTypeBuilderImpl() {
        }

        protected CType.CTypeBuilderImpl self() {
            return this;
        }

        public CType build() {
            return new CType(this);
        }
    }
}
```
{% endcut %}

### Особенности применения со Spring
##### DI
Если у класса помеченного `@Component` единственный конструктор, то можно не вешать на него аннотацию `Autowired`.
Тогда при использовании Lombok'a достаточно повесить `@AllArgsConstructor/@RequiredArgsConstructor`,
а не прописывать конструктор явным образом.

{% cut "Пример" %}
```
@Component
@RequiredArgsConstructor
public class MySpringComponent {
    
    private final OtherSpringComponent otherSpringComponent;
    private final OtherSpringService otherSpringService;
}
```
{% endcut %}

##### Entity-классы.
Для классов-сущностей зачастую использовать `@Data` в сочетании с `@NoArgsConstructor(access = AccessLevel.PACKAGE)`.
При этом главное не забыть исключить из `ToString` и `EqualsAndHashCode` родительские сущности (если они существуют),
иначе любой вызов `toString/equal/hashCode` спровоцирует `StackOverflowException`.

{% cut "Пример" %}
```java
@Data
@Entity(name = "...")
@Table(name = "...")
@NoArgsConstructor(access = AccessLevel.PACKAGE)
public class MyEntity {
    ...
}
```
{% endcut %}

##### DTO-классы.
Начиная с Java 9 Lombok по умолчанию больше не генерирует самостоятельно аннотацию `@ConstructorProperties` над 
конструктором со всеми полями. Есть несколько вариантов:
- прописать `lombok.anyConstructor.addConstructorProperties=true` в файле `lombok.config`. Файл конфигурации при этом желательно класть в общий пакет с dto, чтобы не было применения ненужных аннотаций к не-dto классам.
- Явно прописывать конструктор и эту аннотацию для request-класса, аннотированного `@Value`.
- Использовать `@Data`.

### Запрещённые аннотации
1) [`@SneakyThrows`](https://projectlombok.org/features/SneakyThrows) — позволяет игнорировать checked exception.
Делает магию с байткодом, по опыту это очень больно отлаживать.
2) `@Cleanup` — вместо неё использовать try-with-resources.
3) `@val` и `@var` — автовыведение типов, неактуально с тех пор как `var` появился в Java 10.
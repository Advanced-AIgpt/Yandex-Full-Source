# Как создать XScript-блок нового типа

В некоторых случаях вместо применения технологий распределенного программирования (создания CORBA-сервантов для последующего обращения к ним) из соображений производительности лучше реализовывать нужную функциональность в самом XScript. Для этого можно создавать XScript-блоки новых типов.

Для того чтобы создать новый XScript-блок, необходимо:

1. Установить пакет _libxscript-dev_.
    
1. Собрать динамическую библиотеку на языке C++, в которой должен находиться класс, унаследованный от _xscript::Block_ (`xscript/block.h`).
    
1. В унаследованном от `XScript::Block` классе для добавления функциональности переопределить метод

	```
	virtual XmlDocHelper call(boost::shared_ptr<Context> ctx, boost::any &a) throw (std::exception) = 0;
	```
    
    _`XmlDocHelper`_ – это обертка вокруг xmlDocPtr из библиотеки libxml2.

1. Определить имена методов, которые будут вызываться из блока:

	```inline const std::string& method() const;```
    
1. Определить список параметров вызываемого метода:

	```
	const std::vector<Param*>& params() const;
	```

	Значения _`Param`_ (`xscript/param.h`) можно получить в виде строки следующим методом:

	```
	std::string asString(const Context *ctx) const;
	```
	В классе _`Context`_ есть ссылка на объекты [Request](../concepts/request-ov.md) (`xscript/request.h`) и [State](../concepts/state-ov.md) (`xscript/state.h`), что позволяет обращаться к ним посредством вызовов следующих методов:

	```
	inline Request* request() const;

	inline State* state() const;
    ```

1. Добавить модуль, который будет создавать блоки нового типа. Для этого необходимо унаследоваться от _xscript::Extention_ (`xscript/extention.h`) и переопределить следующие методы:
    
    ```
	//Название модуля, соответствующее названию блока (например, mist)
	virtual const char* name() const = 0;

	//URL XML-нэймспейса, в котором будет находиться блок (например, http://www.yandex.ru/xscript)
	virtual const char* nsref() const = 0;

	//Создание контекста запроса
	virtual void initContext(Context *ctx) = 0;

	//Действия, которые должны быть выполнены после того, как были обработаны все блоки или истекли таймауты на их обработку 
	//(начинает собираться страница, содержащая результаты вызовов блоков)
	virtual void stopContext(Context *ctx) = 0; 

	//Уничтожение контекста запроса
	virtual void destroyContext(Context *ctx) = 0; 
	```
	
	Для добавления каких-либо данных в контекст используются следующие темплейты:

	```
	template<typename T> T Context::param(const std::string &name) const;
	template<typename T> void Context::param(const std::string &name, const T &t);
	```

	Блок создается следующим методом:
	
	```
	virtual std::auto_ptr<Block> createBlock(Xml *owner, xmlNodePtr node) = 0;
	```

В результате выполнения процедуры будет создан XScript-блок нового типа.


### Узнайте больше {#learn-more}
* [Понятие XScript-блока и его типы](../concepts/block-ov.md)

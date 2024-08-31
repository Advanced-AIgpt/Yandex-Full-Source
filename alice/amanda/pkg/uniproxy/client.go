package uniproxy

type Client interface {
	SendText(text string) (*Response, error)
	SendVoice(data []byte) (*Response, error)
	SendCallback(name string, payload interface{}) (*Response, error)
	SendImage(url string) (*Response, error)
}

type client struct {
	settings Settings
}

// TODO: add channel based methods to improve ux

func NewClient(settings Settings) Client {
	return &client{
		settings: settings,
	}
}

func (c *client) SendText(text string) (*Response, error) {
	return c.withConn(func(conn *Conn) (*Response, error) {
		return conn.sendText(text)
	})
}

func (c *client) SendVoice(data []byte) (*Response, error) {
	return c.withConn(func(conn *Conn) (*Response, error) {
		return conn.sendVoice(data)
	})
}

func (c *client) SendCallback(name string, payload interface{}) (*Response, error) {
	return c.withConn(func(conn *Conn) (*Response, error) {
		return conn.sendCallback(name, payload)
	})
}

func (c *client) SendImage(url string) (*Response, error) {
	return c.withConn(func(conn *Conn) (*Response, error) {
		return conn.sendImage(url)
	})
}

func (c *client) withConn(callback func(conn *Conn) (*Response, error)) (*Response, error) {
	conn, err := newConn(c.settings)
	if err != nil {
		return nil, err
	}
	defer func() {
		_ = conn.close()
	}()
	if err := conn.sendSynchronizeState(); err != nil {
		return nil, err
	}
	return callback(conn)
}

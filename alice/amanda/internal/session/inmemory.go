package session

type InMemoryStorage struct {
	memory map[int64]*Session
}

func NewInMemoryStorage() *InMemoryStorage {
	return &InMemoryStorage{
		memory: make(map[int64]*Session),
	}
}

func (storage *InMemoryStorage) Count() (int64, error) {
	return int64(len(storage.memory)), nil
}

func (storage *InMemoryStorage) Load(chatID int64) (*Session, error) {
	if sess, ok := storage.memory[chatID]; ok {
		return sess, nil
	}
	return nil, ErrSessionNotFound
}

func (storage *InMemoryStorage) Save(session *Session) error {
	storage.memory[session.ChatID] = session
	return nil
}

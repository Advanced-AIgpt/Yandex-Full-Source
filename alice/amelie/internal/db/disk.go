package db

import (
	"a.yandex-team.ru/alice/amelie/internal/model"
	"bytes"
	"context"
	"encoding/gob"
	"io/ioutil"
	"log"
	"strings"
)

type DiskDB struct {
	db   *InMemoryDB
	path string
}

func NewDiskDB(path string) DB {
	db := &DiskDB{
		db:   NewInMemoryDB(),
		path: path,
	}
	var err error
	db.db.coll, err = db.load()
	if err != nil {
		log.Fatal(err)
	}
	return db
}

func (d *DiskDB) Count(ctx context.Context) (int64, error) {
	return d.db.Count(ctx)
}

func (d *DiskDB) Load(ctx context.Context, sessionID string) (model.Session, error) {
	return d.db.Load(ctx, sessionID)
}

func (d *DiskDB) Save(ctx context.Context, session model.Session) error {
	if err := d.db.Save(ctx, session); err != nil {
		return err
	}
	return d.save()
}

func (d *DiskDB) load() (map[string]model.Session, error) {
	data, err := ioutil.ReadFile(d.path)
	if err != nil {
		if strings.Contains(err.Error(), "no such file or directory") {
			return map[string]model.Session{}, nil
		}
		return nil, err
	}
	if len(data) == 0 {
		return map[string]model.Session{}, nil
	}
	res := map[string]model.Session{}
	buffer := bytes.NewBuffer(data)
	err = gob.NewDecoder(buffer).Decode(&res)
	return res, err
}

func (d *DiskDB) save() error {
	var buffer bytes.Buffer
	if err := gob.NewEncoder(&buffer).Encode(d.db.coll); err != nil {
		return err
	}
	return ioutil.WriteFile(d.path, buffer.Bytes(), 0644)
}

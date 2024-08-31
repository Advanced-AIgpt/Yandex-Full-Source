package protodumper

import (
	"io/ioutil"
	"os"
	"path"

	"google.golang.org/protobuf/proto"

	"a.yandex-team.ru/library/go/core/xerrors"
)

func DumpToFile(directoryPath, filename string, protoMessage proto.Message) error {
	dirInfo, err := os.Stat(directoryPath)
	if err != nil {
		if os.IsNotExist(err) {
			if err := os.MkdirAll(directoryPath, 0755); err != nil {
				return xerrors.Errorf("can't dump proto message: can't create directory to dump into: %w", err)
			}
		} else {
			return xerrors.Errorf("can't dump proto message: can't find directory to dump into: %w", err)
		}
	}
	if !dirInfo.IsDir() {
		return xerrors.Errorf("can't dump proto message: %s is not a directory", directoryPath)
	}
	protoBytes, err := proto.Marshal(protoMessage)
	if err != nil {
		return xerrors.Errorf("can't dump proto message: %w", err)
	}
	return ioutil.WriteFile(path.Join(directoryPath, filename), protoBytes, 0644)
}

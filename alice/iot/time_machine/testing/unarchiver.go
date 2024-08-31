package testing

import (
	"archive/zip"
	"fmt"
	"io"
	"io/ioutil"
	"os"
	"path/filepath"
	"strings"
)

func Unarchive(srcFile, dstDir string) (err error) {
	r, err := zip.OpenReader(srcFile)
	if err != nil {
		return err
	}
	defer r.Close()

	symlinks := make(map[string]string)

	for _, f := range r.File {
		fpath := filepath.Join(dstDir, f.Name)

		if !strings.HasPrefix(fpath, filepath.Clean(dstDir)+string(os.PathSeparator)) {
			return fmt.Errorf("illegal file path: %s", fpath)
		}

		fi := f.FileInfo()

		switch {
		case fi.IsDir():
			if err := os.MkdirAll(fpath, os.ModePerm); err != nil {
				return err
			}
			continue
		case fi.Mode()&os.ModeSymlink != 0:
			if err = os.MkdirAll(filepath.Dir(fpath), os.ModePerm); err != nil {
				return err
			}

			rc, err := f.Open()
			if err != nil {
				return err
			}
			linkData, err := ioutil.ReadAll(rc)
			if err != nil {
				return err
			}

			if err := rc.Close(); err != nil {
				return err
			}

			symlinks[fpath] = string(linkData) // link file contains relative link
		default:
			if err = os.MkdirAll(filepath.Dir(fpath), os.ModePerm); err != nil {
				return err
			}

			outFile, err := os.OpenFile(fpath, os.O_WRONLY|os.O_CREATE|os.O_TRUNC, f.Mode())
			if err != nil {
				return err
			}

			rc, err := f.Open()
			if err != nil {
				return err
			}

			_, err = io.Copy(outFile, rc)
			if err != nil {
				return err
			}

			if err := outFile.Close(); err != nil {
				return err
			}
			if err := rc.Close(); err != nil {
				return err
			}
		}
	}

	for link := range symlinks {
		err := os.Symlink(symlinks[link], link)
		if err != nil {
			return err
		}
	}

	return nil
}

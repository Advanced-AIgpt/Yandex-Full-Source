package inflector

/*
#include "inflector.h"
#include <stdlib.h>
*/
import "C"
import (
	"fmt"
	"strings"
	"unsafe"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Client struct {
	Logger log.Logger
}

func (i *Client) Inflect(text string, grams []string) (Inflection, error) {
	ctext := C.CString(text)
	defer func() { C.free(unsafe.Pointer(ctext)) }()

	inflection := Inflection{
		Im:   text,
		Rod:  text,
		Dat:  text,
		Vin:  text,
		Tvor: text,
		Pr:   text,
	}

	for _, gram := range grams {
		err := applyGrammar(&inflection, gram, ctext)
		if err != nil {
			i.Logger.Warnf("Unable to inflect %s to %s, error: %v", text, gram, err)
			return Inflection{}, xerrors.Errorf("unable to inflect %s to %s, err: %w", text, gram, err)
		}
	}
	return inflection, nil
}

func applyGrammar(inflection *Inflection, gram string, ctext *C.char) error {
	var err *C.char
	defer func() { C.free(unsafe.Pointer(err)) }()

	cgram := C.CString(gram)
	defer func() { C.free(unsafe.Pointer(cgram)) }()

	cres := C.Inflect(ctext, cgram, &err)
	defer func() { C.free(unsafe.Pointer(cres)) }()

	res := C.GoString(cres)
	if res == "" {
		return nil
	}

	switch gram {
	case Im:
		inflection.Im = res
	case Rod:
		inflection.Rod = res
	case Dat:
		inflection.Dat = res
	case Vin:
		inflection.Vin = res
	case Tvor:
		inflection.Tvor = res
	case Pr:
		inflection.Pr = res
	default:
		return xerrors.Errorf("unknown gram %q", gram)
	}

	if err != nil {
		return xerrors.New(C.GoString(err))
	} else {
		return nil
	}
}

func (i *Client) InflectWithHints(tokens []string, hints []string, grams []string) (Inflection, error) {
	textWithHints := make([]string, 0, len(hints))
	for i, word := range tokens {
		textWithHints = append(textWithHints, fmt.Sprintf("%s{grams=%s}", word, hints[i]))
	}
	return i.Inflect(strings.Join(textWithHints[:], " "), grams)
}

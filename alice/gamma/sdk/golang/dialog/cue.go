package dialoglib

import (
	"bytes"
	"text/template"
)

func ExecuteToString(t *template.Template, args interface{}) (string, error) {
	var tpl bytes.Buffer
	if err := t.Execute(&tpl, args); err != nil {
		return "", err
	}
	return tpl.String(), nil
}

type Cue struct {
	Text  string
	Voice string
}

type CueTemplate struct {
	Text  string
	Voice string

	templateName  string
	textTemplate  *template.Template
	voiceTemplate *template.Template
}

func (cueTemplate *CueTemplate) Compile() (err error) {
	templateName := cueTemplate.templateName
	if cueTemplate.textTemplate, err = template.New(templateName + ".Text").Parse(cueTemplate.Text); err != nil {
		return err
	}
	if cueTemplate.voiceTemplate, err = template.New(templateName + ".Voice").Parse(cueTemplate.Voice); err != nil {
		return err
	}
	return nil
}

func (cueTemplate *CueTemplate) Execute(args interface{}) (cue *Cue, err error) {
	cue = &Cue{}
	if cueTemplate.textTemplate == nil || cueTemplate.voiceTemplate == nil {
		if err = cueTemplate.Compile(); err != nil {
			return nil, err
		}
	}

	if cue.Text, err = ExecuteToString(cueTemplate.textTemplate, args); err != nil {
		return nil, err
	}
	if cue.Voice, err = ExecuteToString(cueTemplate.voiceTemplate, args); err != nil {
		return nil, err
	}
	return cue, nil
}

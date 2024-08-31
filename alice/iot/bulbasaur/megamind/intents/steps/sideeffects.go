package steps

import (
	libnlg "a.yandex-team.ru/alice/library/go/nlg"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
)

type SideEffects struct {
	StackEngineEffects []*scenarios.TStackEngineEffect
	Directives         []*scenarios.TDirective
	NLG                libnlg.NLG
}

func emptySideEffects() SideEffects {
	return SideEffects{
		make([]*scenarios.TStackEngineEffect, 0),
		make([]*scenarios.TDirective, 0),
		nil,
	}
}

func (se *SideEffects) AddStackEngineEffects(effects ...*scenarios.TStackEngineEffect) {
	se.StackEngineEffects = append(se.StackEngineEffects, effects...)
}

func (se *SideEffects) AddDirectives(directives ...*scenarios.TDirective) {
	se.Directives = append(se.Directives, directives...)
}

func (se *SideEffects) SetNLG(nlg libnlg.NLG) {
	se.NLG = nlg
}

func (se *SideEffects) BuildStackEngine(recoveryAction *scenarios.TCallbackDirective) *scenarios.TStackEngine {
	newSession := &scenarios.TStackEngineAction_NewSession{NewSession: &scenarios.TStackEngineAction_TNewSession{}}
	resetAdd := &scenarios.TStackEngineAction_ResetAdd{
		ResetAdd: &scenarios.TStackEngineAction_TResetAdd{
			Effects: se.StackEngineEffects,
			RecoveryAction: &scenarios.TStackEngineAction_TResetAdd_TRecoveryAction{
				Callback: recoveryAction,
			},
		},
	}
	stackEngine := &scenarios.TStackEngine{
		Actions: []*scenarios.TStackEngineAction{
			{Action: newSession},
			{Action: resetAdd},
		},
	}
	return stackEngine
}

func mergeSideEffects(a SideEffects, b SideEffects) SideEffects {
	result := emptySideEffects()
	result.AddDirectives(a.Directives...)
	result.AddDirectives(b.Directives...)
	result.AddStackEngineEffects(a.StackEngineEffects...)
	result.AddStackEngineEffects(b.StackEngineEffects...)
	if a.NLG != nil {
		result.SetNLG(a.NLG)
	}
	if b.NLG != nil {
		result.SetNLG(b.NLG)
	}
	return result
}

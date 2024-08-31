#pragma once

class TNewsBlockSideEffect {
public:
    TNewsBlockSideEffect(TStringBuf name) : Name(name) {
    }

    virtual void ApplySideEffect();

    virtual void UpdateDirective();

private:
    TStringBuf Name;
};

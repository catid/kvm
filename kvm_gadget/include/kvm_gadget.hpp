// Copyright 2020 Christopher A. Taylor

/*
    FIXME
*/

#pragma once

#include "kvm_core.hpp"

namespace kvm {


//------------------------------------------------------------------------------
// KeyboardEmulator

class KeyboardEmulator
{
public:
    ~KeyboardEmulator()
    {
        Shutdown();
    }

    bool Initialize();
    void Shutdown();

    void Send(char ch);

protected:
};


//------------------------------------------------------------------------------
// MouseEmulator

class MouseEmulator
{
public:
    ~MouseEmulator()
    {
        Shutdown();
    }

    bool Initialize();
    void Shutdown();

    void MouseMove(int x, int y);
    void MouseLeftClick(bool down);
    void MouseMiddleClick(bool down);
    void MouseRightClick(bool down);

protected:
};


} // namespace kvm

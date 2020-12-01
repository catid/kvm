// Copyright 2020 Christopher A. Taylor

#include "kvm_gadget.hpp"
#include "kvm_logger.hpp"

namespace kvm {

static logger::Channel Logger("Gadget");


//------------------------------------------------------------------------------
// KeyboardEmulator

bool KeyboardEmulator::Initialize()
{

}

void KeyboardEmulator::Shutdown()
{

}

void KeyboardEmulator::Send(char ch)
{

}


//------------------------------------------------------------------------------
// MouseEmulator

bool MouseEmulator::Initialize()
{

}

void MouseEmulator::Shutdown()
{

}

void MouseEmulator::MouseMove(int x, int y)
{

}

void MouseEmulator::MouseLeftClick(bool down)
{

}

void MouseEmulator::MouseMiddleClick(bool down)
{

}

void MouseEmulator::MouseRightClick(bool down)
{
    
}


} // namespace kvm

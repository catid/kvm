// Copyright 2020 Christopher A. Taylor

#include "kvm_core.hpp"
#include "kvm_logger.hpp"
using namespace kvm;

static logger::Channel Logger("CoreTest");

int main(int argc, char* argv[])
{
    SetCurrentThreadName("Main");

    CORE_UNUSED(argc);
    CORE_UNUSED(argv);

    return kAppSuccess;
}

// Copyright 2020 Christopher A. Taylor

#include "kvm_capture.hpp"
#include "kvm_logger.hpp"
using namespace kvm;

static logger::Channel Logger("CaptureTest");

int main(int argc, char* argv[])
{
    CORE_UNUSED(argc);
    CORE_UNUSED(argv);

    Logger.Info("kvm_capture_test");

    return kAppSuccess;
}

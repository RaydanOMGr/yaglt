#include "test_framework.hpp"

#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

static void segvHandler(int sig) {
    void* frames[64];
    int n = backtrace(frames, 64);
    fprintf(stderr, "SEGV backtrace (%d frames):\n", n);
    backtrace_symbols_fd(frames, n, 2);
    _exit(2);
}

int main() {
    signal(SIGSEGV, segvHandler);
    return ::yaglt::test::runAll();
}

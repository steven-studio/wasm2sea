#pragma once
// TRACE(...) is used throughout node/*.hpp for optional debug logging.
// Flip ENABLE_IR_TRACE to 1 to get the fprintf(stderr, "[IR_TRACE] " ...)
// behavior; left as a no-op by default, same as the original single-file
// ir_bridge.cpp.
#define ENABLE_IR_TRACE 0

#if ENABLE_IR_TRACE
        // #define TRACE(...) fprintf(stderr, "[IR_TRACE] " __VA_ARGS__)
#else
#define TRACE(...) do {} while(0)
#endif

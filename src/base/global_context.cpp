#include "global_context.h"
#include "main_window/glwidget.h"

namespace {
GLWidget* global_context = nullptr;
}

void set_global_context(GLWidget* context) {
    global_context = context;
}

void make_global_context_current() {
    if (global_context) {
        global_context->makeCurrent();
    }
}

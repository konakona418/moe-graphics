#include "Core/App.hpp"

int main() {
    moe::App app;
    app.initialize();
    app.runUntilExit();
    app.shutdown();
    return 0;
}
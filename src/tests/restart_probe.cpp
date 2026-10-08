// restart_probe: minimal "installed app" stand-in for the update E2E.
// On launch it appends one line to the marker file and exits 0, proving
// the Updater's --run-after relaunch really started the new binary.
#include <windows.h>
#include <cstdio>

int main(int argc, char** argv) {
    const char* marker = (argc > 1) ? argv[1] : "restarted.txt";
    FILE* f = nullptr;
    if (fopen_s(&f, marker, "a") == 0 && f) {
        fprintf(f, "restarted pid=%lu\n", (unsigned long)GetCurrentProcessId());
        fclose(f);
    }
    return 0;
}

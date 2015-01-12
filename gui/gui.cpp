#include "MainWindow.hpp"

#include <QApplication>
#include <unistd.h>

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    int logFlags = 0;
    int opt;
    while ((opt = getopt(argc, argv, "t")) != -1) {
        switch (opt) {
            case 't':
                logFlags |= Log_Insns;
                break;
            case 'm':
                logFlags |= Log_MemoryAccesses;
                break;
            default:
                fprintf(stderr, "usage: %s [-t] [rom]\n", argv[0]);
                return 1;
        }
    }
    const char* file = optind >= argc ? "test.bin" : argv[optind];

    try {
        MainWindow main(file, LogFlags(logFlags));
        main.show();

        return app.exec();
    } catch (const char* msg) {
        fprintf(stderr, "error: %s\n", msg);
        return 1;
    }
}

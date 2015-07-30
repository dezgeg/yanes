#include "MainWindow.hpp"

#include <QApplication>
#include <unistd.h>

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    int logFlags = 0;
    long maxFrame = -1;
    int opt;
    while ((opt = getopt(argc, argv, "tml:")) != -1) {
        switch (opt) {
            case 't':
                logFlags |= Log_Insns;
                break;
            case 'm':
                logFlags |= Log_MemoryAccesses;
                break;
            case 'l':
                maxFrame = atol(optarg);
                break;

            default:
                fprintf(stderr, "usage: %s [-t] [rom]\n", argv[0]);
                return 1;
        }
    }
    const char* file = optind >= argc ? "test.bin" : argv[optind];

    try {
        MainWindow main(file, LogFlags(logFlags), maxFrame);
        main.show();

        return app.exec();
    } catch (const char* msg) {
        fprintf(stderr, "error: %s\n", msg);
        return 1;
    }
}

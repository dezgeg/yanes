#pragma once
#include "emu/Gameboy.hpp"
#include "emu/Logger.hpp"
#include "emu/Rom.hpp"

#include <QMainWindow>
#include <QPixmap>
#include <QTimer>
#include <memory>

namespace Ui { class MainWindow; }

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(const char* romFile, bool insnTrace, QWidget *parent = 0);
    ~MainWindow();

private:
    Logger log;
    Rom rom;
    Gameboy gb;

    QTimer* frameTimer;
    QPixmap qtFramebuffer;
    std::unique_ptr<Ui::MainWindow> ui;

    void fillDynamicRegisterTables();
    void updateRegisters();

private slots:
    void timerTick();

    void lcdFocusChanged(bool);
    void lcdKeyEvent(QKeyEvent*);
    void lcdPaintRequested(QPaintEvent*);

    void patternViewerPaintRequested(QPaintEvent*);
    void tileMapViewerPaintRequested(QPaintEvent*);
};

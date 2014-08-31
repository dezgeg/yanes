#pragma once
#include "emu/Gameboy.hpp"
#include "emu/Logger.hpp"
#include "emu/Rom.hpp"

#include <QMainWindow>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QTimer>
#include <memory>

namespace Ui { class MainWindow; }

class GuiLogger : public Logger
{
    Ui::MainWindow* ui;

public:
    GuiLogger(Ui::MainWindow* ui) :
        ui(ui)
    {
    }

    virtual void logImpl(const char* format, ...) override;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(const char* romFile, bool insnTrace, QWidget *parent = 0);
    ~MainWindow();

private:
    std::unique_ptr<Ui::MainWindow> ui;

    GuiLogger log;
    Rom rom;
    Gameboy gb;

    QTimer* frameTimer;
    QPixmap qtFramebuffer;

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

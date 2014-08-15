#include "MainWindow.hpp"
#include "ui_MainWindow.h"
#include "emu/Utils.hpp"

#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QString>
#include <utility>
using namespace std;

static const pair<unsigned, QString> lcdRegs[] = {
    { 0xff40, QStringLiteral("LCDC") },
    { 0xff41, QStringLiteral("STAT") },
    { 0xff42, QStringLiteral("SCY") },
    { 0xff43, QStringLiteral("SCX") },
    { 0xff44, QStringLiteral("LY") },
    { 0xff45, QStringLiteral("LYC") },
    { 0xff46, QStringLiteral("DMA") },
    { 0xff47, QStringLiteral("BGP") },
    { 0xff4a, QStringLiteral("WY") },
    { 0xff4b, QStringLiteral("WX") },
};

static const QString irqNames[] = {
    QStringLiteral("V-Blank"),
    QStringLiteral("LCD STAT"),
    QStringLiteral("Timer"),
    QStringLiteral("Serial"),
    QStringLiteral("Joypad"),
};

void MainWindow::setupUi()
{
    QFont font("Monospace");
    font.setStyleHint(QFont::TypeWriter);

    for (unsigned i = 0; i < arraySize(lcdRegs); i++) {
        QLabel* label = new QLabel(lcdRegs[i].second);
        ui->lcdRegsFormLayout->setWidget(i, QFormLayout::LabelRole, label);
        QLineEdit* edit = new QLineEdit(QStringLiteral("F0"));
        edit->setFont(font);
        ui->lcdRegsFormLayout->setWidget(i, QFormLayout::FieldRole, edit);
    }

    for (unsigned i = 0; i < arraySize(irqNames); i++) {
        ui->irqsFormLayout->addWidget(new QLabel(irqNames[i]), i, 0);

        QPushButton* enabledBtn = new QPushButton(QStringLiteral("Enabled"));
        ui->irqsFormLayout->addWidget(enabledBtn, i, 1);

        QPushButton* pendingBtn = new QPushButton(QStringLiteral("Pending"));
        ui->irqsFormLayout->addWidget(pendingBtn, i, 2);
    }

    qtFramebuffer.fill(Qt::black);
    QGraphicsScene* scene = new QGraphicsScene(this);
    fbSceneItem = scene->addPixmap(qtFramebuffer);
    scene->setSceneRect(qtFramebuffer.rect());
    ui->lcdGraphicsView->setScene(scene);
    ui->lcdGraphicsView->scale(2, 2);
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    log(false),
    rom(&log, "test.bin"),
    gb(&log, &rom),
    frameTimer(new QTimer(this)),
    qtFramebuffer(160, 144),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setupUi();
    connect(frameTimer, SIGNAL(timeout()), this, SLOT(tick()));
    frameTimer->start(17);
}

void MainWindow::tick()
{
    static const QVector<QRgb> monochromeToRgb = {
        qRgb(0, 0, 0),
        qRgb(255/3, 255/3, 255/3),
        qRgb(2*255/3, 2*255/3, 2*255/3),
        qRgb(255, 255, 255),
    };

    gb.runFrame();

    QImage image((const uchar*)gb.getGpu()->getFramebuffer(),
                 ScreenWidth, ScreenHeight, QImage::Format_Indexed8);
    image.setColorTable(monochromeToRgb); // TODO: copies?
    qtFramebuffer.convertFromImage(image);

    fbSceneItem->setPixmap(qtFramebuffer); // TODO: does this recopy etc...?
}

MainWindow::~MainWindow()
{
}

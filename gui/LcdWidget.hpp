#pragma once

#include "emu/Platform.hpp"
#include <QWidget>
#include <QGLWidget>
#include <QtOpenGL>
#include <functional>

class LcdWidget : public QGLWidget {
Q_OBJECT
    QGLShader* vertexShader;
    QGLShader* fragmentShader;
    QGLShaderProgram* shaderProgram;
    QOpenGLTexture xAxisGridTexture;
    QOpenGLTexture yAxisGridTexture;

    QOpenGLTexture texture;
    Byte* textureData;
    QSize textureSize;

    QOpenGLTexture secondaryTexture;
    Byte* secondaryTextureData;
    QSize secondaryTextureSize;

    char const* fragmentShaderFile;
    std::function<void(LcdWidget*)> drawCallback;

    static QGLFormat& createGLFormat();
    static void setupTexture(QOpenGLTexture& glTexture);
    void makeGridTexture(QOpenGLTexture& gridTexture, int size);

protected:
    virtual void initializeGL() override;
    virtual void resizeGL(int w, int h) override;
    virtual void paintGL() override;

    virtual void focusInEvent(QFocusEvent*) override {
        emit focusChanged(true);
    }
    virtual void focusOutEvent(QFocusEvent*) override {
        emit focusChanged(false);
    }
    virtual void paintEvent(QPaintEvent* e) override {
        //emit paintRequested(e);
        updateGL();
    }
    virtual void keyPressEvent(QKeyEvent* e) override {
        emit keyEvent(e);
    }
    virtual void keyReleaseEvent(QKeyEvent* e) override {
        emit keyEvent(e);
    }

public:

    explicit LcdWidget(QWidget* parent) :
            QGLWidget(createGLFormat(), parent),
            xAxisGridTexture(QOpenGLTexture::Target2D),
            yAxisGridTexture(QOpenGLTexture::Target2D),
            texture(QOpenGLTexture::Target2D),
            secondaryTexture(QOpenGLTexture::Target2D) {
        setFocusPolicy(Qt::StrongFocus);
    }

    void init(Byte* textureData, QSize size, Byte* secondaryTextureData, QSize secondaryTextureSize,
              char const* shaderFile,
              std::function<void(LcdWidget*)> drawCallback = nullptr) {
        this->textureData = textureData;
        this->textureSize = size;
        this->secondaryTextureData = secondaryTextureData;
        this->secondaryTextureSize = secondaryTextureSize;
        this->fragmentShaderFile = shaderFile;
        this->drawCallback = drawCallback;
    }

    QGLShaderProgram* getShaderProgram() { return shaderProgram; }

    virtual ~LcdWidget() {
        makeCurrent();
        texture.destroy();
    }

signals:
    void focusChanged(bool in);
    void keyEvent(QKeyEvent*);
};

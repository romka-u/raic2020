#pragma once

#include <QApplication>
#include <QPainter>
#include <QGraphicsScene>
#include <QScreen>
#include <QGraphicsView>
#include <QDesktopWidget>
#include <QGraphicsItem>
#include <QKeyEvent>
#include <QMainWindow>

#include <memory>
#include <functional>
#include <cmath>
#include <iostream>
#include <vector>

namespace {

QPen whitePen(Qt::white);
QPen darkWhitePen(QColor(222, 222, 222));
QPen orangePen(QColor(255, 127, 0));
QPen yellowPen(Qt::yellow);
QPen darkOrangePen(QColor(127, 55, 0));
QPen darkBluePen(QColor(0, 0, 100));
QPen redPen(Qt::red);
QPen thickRedPen(QBrush(Qt::red), 2.0);
QPen thickBlackPen(QBrush(Qt::black), 3.0);
QPen greenPen(Qt::green);
QPen darkGreenPen(QColor(0, 90, 0));
QPen bluePen(Qt::blue);
QPen thickBluePen(QBrush(Qt::blue), 2.0);
QPen blackPen(Qt::black);
QPen transparentPen(Qt::transparent);
QPen magentaPen(Qt::magenta);
QPen palePen(QColor(0xbc, 0xbd, 0xba));
QPen grayPen(QColor(96, 96, 96));
QPen lightBluePen(QColor(166, 202, 240));
QPen darkYellowPen(QColor(100, 100, 0));
QPen darkRedPen(QColor(100, 0, 0));
QPen lightGreenPen(QColor(139, 250, 117));
QPen balloonPen(QColor(0xbc, 0xbd, 0xba, 222));

QBrush whiteBrush(Qt::white);
QBrush darkWhiteBrush(QColor(222, 222, 222));
QBrush orangeBrush(QColor(255, 127, 0));
QBrush darkYellowBrush(QColor(100, 100, 0));
QBrush yellowBrush(Qt::yellow);
QBrush redBrush(Qt::red);
QBrush darkOrangeBrush(QColor(127, 55, 0));
QBrush darkBlueBrush(QColor(0, 0, 100));
QBrush darkRedBrush(QColor(100, 0, 0));
QBrush greenBrush(Qt::green);
QBrush blueBrush(Qt::blue);
QBrush blackBrush(Qt::black);
QBrush darkGrayBrush(QColor(47,44,45));
QBrush paleBrush(QColor(0xbc, 0xbd, 0xba));
QBrush grayBrush(QColor(87,84,85));
QBrush transparentBrush(Qt::transparent);
QBrush magentaBrush(Qt::magenta);
QBrush lightBlueBrush(QColor(159, 194, 229));
QBrush balloonBrush(QColor(0xbc, 0xbd, 0xba, 222));

std::vector<QColor> playerColors = {
    QColor(0, 55, 127),
    QColor(0, 127, 55),
    QColor(127, 0, 55),
    QColor(215, 110, 0)
};

std::vector<QBrush> brushesPerPlayer = {
    QBrush(playerColors[0]),
    QBrush(playerColors[1]),
    QBrush(playerColors[2]),
    QBrush(playerColors[3])
};

std::vector<QPen> pensPerPlayer = {
    QPen(playerColors[0]),
    QPen(playerColors[1]),
    QPen(playerColors[2]),
    QPen(playerColors[3])
};


}

class CustomView : public QGraphicsView {
public:
    CustomView(QGraphicsScene *scene, QWidget *parent = nullptr) : QGraphicsView(scene, parent) {}

    bool paused() const {
        return paused_;
    }

    void mousePressEvent(QMouseEvent* event) {
        if (onMouseClick_)
            onMouseClick_(*event, event->x(), event->y());
    }

    void keyPressEvent(QKeyEvent* event) {
        if (event->key() == Qt::Key_Space) {
            paused_ = !paused_;
        } else if (event->key() == Qt::Key_Equal) {
            scalex *= SCALE_COEFF;
            scaley *= SCALE_COEFF;
        } else if (event->key() == Qt::Key_Minus) {
            scalex /= SCALE_COEFF;
            scaley /= SCALE_COEFF;
        } else if (event->key() == Qt::Key_Left || event->key() == Qt::Key_H) {
            movex -= MOVE_COEFF / scalex;
        } else if (event->key() == Qt::Key_Right || event->key() == Qt::Key_L) {
            movex += MOVE_COEFF / scalex;
        } else if (event->key() == Qt::Key_Up || event->key() == Qt::Key_K) {
            movey -= MOVE_COEFF / scaley;
        } else if (event->key() == Qt::Key_Down || event->key() == Qt::Key_J) {
            movey += MOVE_COEFF / scaley;
        }

        // std::cerr << movex << " " << movey << " " << scalex << " " << scaley << std::endl;

        if (onKeyPress_)
            onKeyPress_(*event);
    }

    void centerOn(double x, double y) {
        movex = x;
        movey = y;
    }

    void setOnMouseClick(std::function<void(const QMouseEvent&, double, double)> onMouseClick) {
        onMouseClick_ = onMouseClick;
    }

    void setOnKeyPress(std::function<void(const QKeyEvent&)> onKeyPress) {
        onKeyPress_ = onKeyPress;
    }

    void resetView() {
        scalex = 0.12;
        scaley = -0.12;
        movex = 3588;
        movey = 141;
    }

    static constexpr double SCALE_COEFF = 1.08;
    static constexpr double MOVE_COEFF = 10;
    double scalex = 0.12;
    double scaley = -0.12;
    double movex = 3588;
    double movey = 141;
    bool paused_ = false;
    std::function<void(const QMouseEvent&, double, double)> onMouseClick_;
    std::function<void(const QKeyEvent&)> onKeyPress_;
};


class Visualizer {
public:
    Visualizer() : pixmapItem(nullptr), height_(0), width_(0) {
        static char arg0[] = "Visualizer";
        static char* argv[] = { &arg0[0], NULL };
        static int argc = (int)(sizeof(argv) / sizeof(argv[0])) - 1;
        QApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
        // QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
        app = new QApplication(argc, argv);
        // app->setAttribute(Qt::AA_UseHighDpiPixmaps);
        // app->setAttribute(Qt::AA_DisableHighDpiScaling);
        scene = new QGraphicsScene();
        view = new CustomView(scene);
        app->processEvents();
    }

    void resetView() {
        view->resetView();
    }

    void setSize(int height, int width) {
        if (height_ == height && width_ == width) return;
        if (height <= 0 || width <= 0)
            throw std::runtime_error("height and width should be greater than 0");
        height_ = height;
        width_ = width;
        scene->setSceneRect(0, 0, height, width);
        pixmap.reset(new QPixmap(height, width));
        scene->clear();
        pixmapItem = scene->addPixmap(*pixmap);
        view->setFixedSize(height, width);
        view->show();
        view->raise();
    }

    void process() {
        if (width_ > 0 && height_ > 0) {
            pixmapItem->setPixmap(*pixmap);
            scene->update();
        }
        app->processEvents();
    }

    void centerOn(double x, double y) {
        view->centerOn(x - width_ * 0.5, y - height_ * 0.5);
    }

    bool paused() const {
        return view->paused();
    }

    void setOnMouseClick(std::function<void(const QMouseEvent&, double, double, double, double)> onMouseClick) {
        onMouseClick_ = onMouseClick;

        view->setOnMouseClick([this] (const QMouseEvent& event, double screenX, double screenY) {
            const double worldX = (screenX - width_ * 0.5) / view->scalex + width_ * 0.5 + view->movex;
            const double worldY = (screenY - height_ * 0.5) / view->scaley + height_ * 0.5 + view->movey;
            onMouseClick_(event, screenX, screenY, worldX, worldY);
        });
    }

    void setOnKeyPress(std::function<void(const QKeyEvent&)> onKeyPress) {
        onKeyPress_ = onKeyPress;
        view->setOnKeyPress([this] (const QKeyEvent& event) {
            onKeyPress_(event);
        });
    }

    QPainter p; // public painter

    ~Visualizer() {
        app->quit();
    }

    void switchToWindow() {
        p.end();
        p.begin(pixmap.get());
        p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing);
    }

    void adjust() {
        p.begin(pixmap.get());
        p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing);
        p.translate((-view->movex - width_ * 0.5) * view->scalex + width_ * 0.5,
                    (-view->movey - height_ * 0.5) * view->scaley + height_ * 0.5);
        p.scale(view->scalex, view->scaley);
    }

    void switchToWorld() {
        p.end();
        adjust();
    }

    QApplication* app;

private:    
    QGraphicsScene* scene;
    CustomView* view;
    std::unique_ptr<QPixmap> pixmap;
    QGraphicsPixmapItem* pixmapItem; // will be removed by scene
    int height_, width_;
    std::function<void(const QMouseEvent&, double, double, double, double)> onMouseClick_;
    std::function<void(const QKeyEvent&)> onKeyPress_;
    friend class RenderCycle;
};

class RenderCycle {
public:
    RenderCycle(Visualizer& visualizer) : visualizer_(&visualizer) {
        while (visualizer.paused())
            visualizer.process();

        visualizer.pixmap->fill(Qt::black);
        
        visualizer.adjust();
    }

    ~RenderCycle() {
        visualizer_->p.end();
        visualizer_->process();
    }

private:
    Visualizer* visualizer_;
};

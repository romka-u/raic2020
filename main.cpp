#include "DebugInterface.hpp"
#include "MyStrategy.h"
#include "TcpStream.hpp"
#include "model/Model.hpp"
#include <memory>
#include <string>
#include <thread>

bool running, exited;
MyStrategy myStrategy;

class Runner {
public:
    Runner(const std::string& host, int port, const std::string& token)
    {
        std::shared_ptr<TcpStream> tcpStream(new TcpStream(host, port));
        inputStream = getInputStream(tcpStream);
        outputStream = getOutputStream(tcpStream);
        outputStream->write(token);
        outputStream->flush();
    }
    void run()
    {
        DebugInterface debugInterface(inputStream, outputStream);
        while (true) {
            auto message = ServerMessage::readFrom(*inputStream);
            if (auto getActionMessage = std::dynamic_pointer_cast<ServerMessage::GetAction>(message)) {
                ClientMessage::ActionMessage(myStrategy.getAction(getActionMessage->playerView, getActionMessage->debugAvailable ? &debugInterface : nullptr)).writeTo(*outputStream);
                outputStream->flush();
            } else if (auto finishMessage = std::dynamic_pointer_cast<ServerMessage::Finish>(message)) {
                running = false;
                break;
            } else if (auto debugUpdateMessage = std::dynamic_pointer_cast<ServerMessage::DebugUpdate>(message)) {
                myStrategy.debugUpdate(debugUpdateMessage->playerView, debugInterface);
                ClientMessage::DebugUpdateDone().writeTo(*outputStream);
                outputStream->flush();
            }
        }
    }

private:
    std::shared_ptr<InputStream> inputStream;
    std::shared_ptr<OutputStream> outputStream;
};

int main(int argc, char* argv[])
{
    running = true;
    std::string host = argc < 2 ? "127.0.0.1" : argv[1];
    int port = argc < 3 ? 31001 : atoi(argv[2]);
    std::string token = argc < 4 ? "0000000000000000" : argv[3];
    
    #ifdef DEBUG 
    std::thread runner([host, port, token]() { Runner(host, port, token).run(); });

    v.setSize(1700, 1000);
    drawTargets = true;
    v.setOnKeyPress([](const QKeyEvent& ev) {
        clickedPointWorld = Vec2Float(1e9, 1e9);
        if (ev.key() == Qt::Key_Escape) { exited = true;  }
        if (ev.key() == Qt::Key_A) { drawMyField = !drawMyField;  }
        if (ev.key() == Qt::Key_S) { drawOppField = !drawOppField;  }
        if (ev.key() == Qt::Key_T) { drawTargets = !drawTargets;  }
        if (ev.key() == Qt::Key_D) { drawInfMap = !drawInfMap;  }
        if (ev.key() == Qt::Key_U) { colorBySquads = !colorBySquads;  }
        if (ev.key() == Qt::Key_R) { v.resetView();  }
        if (ev.key() == Qt::Key_C) { currentDrawTick = max(0, currentDrawTick - 1);  }
        if (ev.key() == Qt::Key_V) { currentDrawTick = min(maxDrawTick, currentDrawTick + 1); }
    });

    v.setOnMouseClick([](const QMouseEvent& ev, double sx, double sy, double wx, double wy) {
        clickedPointWorld = Vec2Float(wx, wy);
        clickedPointScreen = Vec2Float(sx, sy);
    });

    bool moved = false;
    while (running || !exited) {
        if (!moved) {
            auto w = v.app->activeWindow();
            if (w) {
                QDesktopWidget *widget = QApplication::desktop();
                auto g = QGuiApplication::screens()[1]->availableGeometry();
                std::cerr << g.x() << " " << g.y() << std::endl;
                w->move(g.x(), g.y());
                moved = true;
            }
        }
        RenderCycle r(v);
        draw();
    }
    runner.join();
    #else
    Runner(host, port, token).run();
    #endif

    return 0;
}
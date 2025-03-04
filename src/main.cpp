#include <QApplication>
#include <QMainWindow>
#include <QPainter>
#include <QScreen>

class MyWindow : public QMainWindow {
public:
    MyWindow(QWidget *parent = nullptr) : QMainWindow(parent) {}

protected:
    void paintEvent(QPaintEvent *event) override {
        QPainter painter(this);

        // Get current window size
        int winWidth = width();
        int winHeight = height();

        // Set background color
        painter.fillRect(rect(), Qt::white);

        // Scale factor based on window size
        float scale = std::min(winWidth, winHeight) / 400.0f;

        // Draw a red rectangle
        painter.setBrush(Qt::red);
        painter.drawRect(50 * scale, 50 * scale, 100 * scale, 50 * scale);

        // Draw a blue circle
        painter.setBrush(Qt::blue);
        painter.drawEllipse(200 * scale, 50 * scale, 50 * scale, 50 * scale);

        // Draw scaled text
        QFont font = painter.font();
        font.setPointSizeF(10 * scale);
        painter.setFont(font);
        painter.setPen(Qt::black);
        painter.drawText(50 * scale, 150 * scale, "Hello, Qt World!");

        // Draw a green line
        painter.setPen(QPen(Qt::green, 2 * scale));
        painter.drawLine(50 * scale, 200 * scale, 300 * scale, 200 * scale);
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    MyWindow window;

    // Get the screen size
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    int screenWidth = screenGeometry.width();
    int screenHeight = screenGeometry.height();

    // Calculate half size for initial window size
    int windowSize = std::min(screenWidth, screenHeight) / 2;
    window.resize(windowSize, windowSize);

    // Center the window on the screen
    int xPos = (screenWidth - windowSize) / 2;
    int yPos = (screenHeight - windowSize) / 2;
    window.move(xPos, yPos);

    window.setMinimumSize(300, 300);        // Ensure a reasonable minimum size
    window.setWindowTitle("Qt Hello World");

    // Explicitly set the main window for the application
    app.setActiveWindow(&window);

    window.show();

    return app.exec();
}

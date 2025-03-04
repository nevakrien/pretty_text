#include <QApplication>
#include <QScreen>
#include <QRect>
#include <QVBoxLayout>
#include <QWidget>
#include <QTextEdit>
#include <QString>
#include <QWheelEvent>  // Required for QWheelEvent
#include <QKeyEvent>    // Required for keyPressEvent
#include <QDebug>       // For debug printing

// CustomTextEdit class to handle custom scroll, zoom, and Ctrl+S behavior
class CustomTextEdit : public QTextEdit {
public:
    CustomTextEdit(QWidget *parent = nullptr) : QTextEdit(parent), defaultFontSize(fontPointSize()) {
        setPlainText("Welcome to the custom text editor!\n"
                     "Use Ctrl + Scroll to Zoom In/Out.\n"
                     "Use Alt + Scroll for regular scrolling.\n"
                     "Press Ctrl + S to save (not implemented yet).\n"
                     "Feel free to type and scroll around.");
    }

protected:
    // Handle mouse wheel events for scroll and zoom
    void wheelEvent(QWheelEvent *event) override {
        // Handle Ctrl + Scroll for Zoom
        if (event->modifiers() & Qt::ControlModifier) {
            int delta = event->angleDelta().y();
            if (delta > 0) {
                zoomIn();  // Zoom In
            } else if (delta < 0) {
                zoomOut();  // Zoom Out
            }
        }
        else {
            QTextEdit::wheelEvent(event);
        }
    }

    // Handle key press events for shortcuts like Ctrl + S
    void keyPressEvent(QKeyEvent *event) override {
        if (event->modifiers() & Qt::ControlModifier && event->key() == Qt::Key_S) {
            qDebug() << "Ctrl + S pressed! (Save not implemented yet)";
            event->accept();  // Mark event as handled
        } else {
            QTextEdit::keyPressEvent(event);  // Default handling for other keys
        }
    }

private:
    int defaultFontSize;  // Store the default font size

    // Zoom In by increasing the font size
    void zoomIn() {
        QFont font = this->font();
        int currentSize = font.pointSize();
        font.setPointSize(currentSize + 1);  // Increase font size
        setFont(font);
    }

    // Zoom Out by decreasing the font size
    void zoomOut() {
        QFont font = this->font();
        int currentSize = font.pointSize();
        if (currentSize > 1) {  // Prevent font size from becoming zero or negative
            font.setPointSize(currentSize - 1);  // Decrease font size
            setFont(font);
        }
    }
};

// Main window class to hold the text editor
class MyWindow : public QWidget {
public:
    MyWindow(QWidget *parent = nullptr) : QWidget(parent) {
        QVBoxLayout *layout = new QVBoxLayout(this);
        CustomTextEdit *textEdit = new CustomTextEdit(this);
        layout->addWidget(textEdit);
        setLayout(layout);
    }
};

// Main function to set up and display the window
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    MyWindow window;

    // Get screen geometry and center the window
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    int screenWidth = screenGeometry.width();
    int screenHeight = screenGeometry.height();

    // Set window size to half of the smaller screen dimension
    int windowSize = std::min(screenWidth, screenHeight) / 2;
    window.resize(windowSize, windowSize);

    // Center the window on the screen
    int xPos = (screenWidth - windowSize) / 2;
    int yPos = (screenHeight - windowSize) / 2;
    window.move(xPos, yPos);

    // Set minimum size for the window
    window.setMinimumSize(300, 300);
    window.setWindowTitle("Custom Text Editor with Zoom and Save Shortcut");
    window.show();

    return app.exec();
}

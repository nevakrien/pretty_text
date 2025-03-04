#include <QApplication>
#include <QScreen>
#include <QRect>
#include <QVBoxLayout>
#include <QWidget>
#include <QTextEdit>
#include <QString>
#include <QWheelEvent>   // Required for QWheelEvent
#include <QKeyEvent>     // Required for keyPressEvent
#include <QDebug>        // For debug printing
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QTimer>
#include <cmath>         // for sin/cos if needed

/********************************************************************
 * CustomTextEdit: same as your original code, with Ctrl+Scroll zoom,
 * Ctrl+S debug message, minimum changes.
 ********************************************************************/
class CustomTextEdit : public QTextEdit {
public:
    CustomTextEdit(QWidget *parent = nullptr)
        : QTextEdit(parent), defaultFontSize(fontPointSize())
    {
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
                zoomIn();
            } else if (delta < 0) {
                zoomOut();
            }
        }
        else {
            // Default scroll
            QTextEdit::wheelEvent(event);
        }
    }

    // Handle key press events for shortcuts like Ctrl + S
    void keyPressEvent(QKeyEvent *event) override {
        if (event->modifiers() & Qt::ControlModifier && event->key() == Qt::Key_S) {
            qDebug() << "Ctrl + S pressed! (Save not implemented yet)";
            event->accept();
        } else {
            QTextEdit::keyPressEvent(event);  // Default handling
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
        if (currentSize > 1) {
            font.setPointSize(currentSize - 1);  // Decrease font size
            setFont(font);
        }
    }
};

/********************************************************************
 * SimpleOverlay: A QOpenGLWidget that draws a rotating semi-transparent
 * triangle above the text editor. (Minimal example)
 ********************************************************************/
class SimpleOverlay : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit SimpleOverlay(QWidget *parent = nullptr)
        : QOpenGLWidget(parent)
        , angle(0.0f)
    {
        // Repaint ~ 60 FPS
        startTimer(16);

        // Let mouse clicks pass through to underlying widget
        setAttribute(Qt::WA_TransparentForMouseEvents);

        // Ensure this widget stays on top visually
        setAttribute(Qt::WA_AlwaysStackOnTop);
    }

protected:
    void initializeGL() override {
        initializeOpenGLFunctions();
        qDebug() << "[SimpleOverlay] initializeGL() called.";
    }

    void resizeGL(int w, int h) override {
        glViewport(0, 0, w, h);
        qDebug() << "[SimpleOverlay] resizeGL()" << w << "x" << h;
    }

    void paintGL() override {
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Transparent clear
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Very simple rotating triangle
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        // Use an orthographic projection [-1..1] in both X, Y
        glOrtho(-1, 1, -1, 1, -1, 1);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glRotatef(angle, 0.0f, 0.0f, 1.0f);  // spin around Z axis
        angle += 1.0f;  // increment for next frame

        // Semi-transparent
        glColor4f(1.0f, 0.0f, 0.0f, 0.5f);

        glBegin(GL_TRIANGLES);
          glVertex2f(-0.5f, -0.4f);
          glVertex2f( 0.5f, -0.4f);
          glVertex2f( 0.0f,  0.6f);
        glEnd();
    }

    // Timer event to force repaints
    void timerEvent(QTimerEvent *) override {
        update(); // schedule next paint
    }

private:
    float angle;
};

/********************************************************************
 * MyWindow: Holds a container in which we place:
 *  1) CustomTextEdit
 *  2) SimpleOverlay  (on top)
 ********************************************************************/
class MyWindow : public QWidget {
    Q_OBJECT
public:
    MyWindow(QWidget *parent = nullptr) : QWidget(parent)
    {
        QVBoxLayout *layout = new QVBoxLayout(this);
        container = new QWidget(this);
        layout->addWidget(container);
        setLayout(layout);

        // Create the text editor as a child of 'container'
        textEdit = new CustomTextEdit(container);
        textEdit->setGeometry(0, 0, container->width(), container->height());
        textEdit->show();

        // Create the overlay as a child of 'container'
        overlay = new SimpleOverlay(container);
        overlay->setGeometry(textEdit->geometry());
        overlay->show();
        overlay->raise();

        // Listen for resize events on the container
        container->installEventFilter(this);
    }

protected:
    bool eventFilter(QObject *obj, QEvent *e) override
    {
        if (obj == container && e->type() == QEvent::Resize) {
            QResizeEvent *re = static_cast<QResizeEvent*>(e);
            // Match geometry for both textEdit and overlay
            textEdit->setGeometry(0, 0, re->size().width(), re->size().height());
            overlay->setGeometry(textEdit->geometry());
            overlay->raise();
        }
        return QWidget::eventFilter(obj, e);
    }

private:
    QWidget        *container = nullptr;
    CustomTextEdit *textEdit  = nullptr;
    SimpleOverlay  *overlay   = nullptr;
};

// Required because MyWindow is a QObject subclass in a single file
#include "main.moc"

/********************************************************************
 * main(): same logic to center the window on the screen, etc.
 ********************************************************************/
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    MyWindow window;

    // Center the window on the screen
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    int screenWidth = screenGeometry.width();
    int screenHeight = screenGeometry.height();

    // Set window size to half of the smaller screen dimension
    int windowSize = std::min(screenWidth, screenHeight) / 2;
    window.resize(windowSize, windowSize);

    // Center the window
    int xPos = (screenWidth - windowSize) / 2;
    int yPos = (screenHeight - windowSize) / 2;
    window.move(xPos, yPos);

    // Set minimum size
    window.setMinimumSize(300, 300);
    window.setWindowTitle("Custom Text Editor + Simple OpenGL Overlay");
    window.show();

    return app.exec();
}

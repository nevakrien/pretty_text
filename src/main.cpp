#include <QApplication>
#include <QMainWindow>
#include <QPainter>
#include <QScreen>
#include <QOpenGLWidget>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFunctions>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QTimer>
#include <QStackedWidget>

// Handles the red square drawing
class SquareWidget : public QWidget {
public:
    SquareWidget(QWidget *parent = nullptr) : QWidget(parent) {
        setAttribute(Qt::WA_TranslucentBackground);
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // Transparent background
        painter.fillRect(rect(), Qt::transparent);

        // Draw a red square
        float scale = std::min(width(), height()) / 200.0f;
        painter.setBrush(Qt::red);
        painter.drawRect(0, 0, 100 * scale, 50 * scale);
    }
};

// Handles OpenGL-based circle drawing with a white background
class CircleWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    QOpenGLShaderProgram shaderProgram;
    QOpenGLBuffer vertexBuffer;
    QOpenGLVertexArrayObject vao;

public:
    CircleWidget(QWidget *parent = nullptr) : QOpenGLWidget(parent) {
        setAttribute(Qt::WA_TranslucentBackground);
    }

protected:
    void initializeGL() override {
        initializeOpenGLFunctions();

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glClearColor(1, 1, 1, 1);  // White background for OpenGL area

        const char *vertexShaderSrc = R"(
            #version 330 core
            layout(location = 0) in vec2 aPos;
            void main() {
                gl_Position = vec4(aPos, 0.0, 1.0);
            }
        )";

        const char *fragmentShaderSrc = R"(
            #version 330 core
            out vec4 FragColor;

            uniform vec2 u_resolution;
            uniform vec2 u_center;
            uniform float u_radius;
            uniform vec4 u_color;

            void main() {
                vec2 uv = gl_FragCoord.xy / u_resolution;
                vec2 pos = (uv - u_center) * u_resolution;

                float distSquared = dot(pos, pos);
                float radiusSquared = u_radius * u_radius;

                if (distSquared <= radiusSquared) {
                    FragColor = u_color;  // Inside the circle
                } else {
                    FragColor = vec4(0.0, 0.0, 0.0, 0.0);  // Transparent outside
                }
            }
        )";

        shaderProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSrc);
        shaderProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSrc);
        shaderProgram.link();

        GLfloat vertices[] = {
            -1.0f, -1.0f,
             1.0f, -1.0f,
            -1.0f,  1.0f,
             1.0f,  1.0f,
        };

        vao.create();
        vao.bind();

        vertexBuffer.create();
        vertexBuffer.bind();
        vertexBuffer.allocate(vertices, sizeof(vertices));

        shaderProgram.bind();
        shaderProgram.enableAttributeArray(0);
        shaderProgram.setAttributeBuffer(0, GL_FLOAT, 0, 2);
    }

    void paintGL() override {
        glClear(GL_COLOR_BUFFER_BIT);  // Clear with white background

        shaderProgram.bind();

        shaderProgram.setUniformValue("u_resolution", QVector2D(width(), height()));
        shaderProgram.setUniformValue("u_center", QVector2D(0.5f, 0.5f));
        shaderProgram.setUniformValue("u_radius", 80.0f);
        shaderProgram.setUniformValue("u_color", QVector4D(0.0f, 0.5f, 1.0f, 1.0f));

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
};

// Main window for the application
class MyWindow : public QMainWindow {
public:
    MyWindow(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Qt with Reusable Widgets");

        QWidget *centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);

        QGridLayout *layout = new QGridLayout(centralWidget);

        QLabel *nativeLabel = new QLabel(tr("Red Square"));
        nativeLabel->setAlignment(Qt::AlignHCenter);
        QLabel *openGLLabel = new QLabel(tr("OpenGL Circle"));
        openGLLabel->setAlignment(Qt::AlignHCenter);

        SquareWidget *squareWidget = new SquareWidget(this);
        CircleWidget *circleWidget = new CircleWidget(this);

        layout->addWidget(circleWidget, 0, 0, 1, 2);  // Spans two columns
        layout->addWidget(squareWidget, 0, 0);         // Overlaps part of the circle

        layout->addWidget(nativeLabel, 1, 0);
        layout->addWidget(openGLLabel, 1, 1);

        QTimer *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, squareWidget, QOverload<>::of(&SquareWidget::update));
        connect(timer, &QTimer::timeout, circleWidget, QOverload<>::of(&CircleWidget::update));
        timer->start(50);
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    MyWindow window;

    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    int screenWidth = screenGeometry.width();
    int screenHeight = screenGeometry.height();

    int windowSize = std::min(screenWidth, screenHeight) / 2;
    window.resize(windowSize, windowSize);

    int xPos = (screenWidth - windowSize) / 2;
    int yPos = (screenHeight - windowSize) / 2;
    window.move(xPos, yPos);

    window.setMinimumSize(300, 300);

    window.show();

    return app.exec();
}

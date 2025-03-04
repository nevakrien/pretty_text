#include <QApplication>
#include <QScreen>
#include <QRect>
#include <QVBoxLayout>
#include <QWidget>

#include <QTextEdit>
#include <QFile>
#include <QTextStream>

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QTimer>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QDebug>
#include <QTextCursor>
#include <QTextBlock>
#include <QAbstractTextDocumentLayout>
#include <cmath>

// -------------------- TextEditing --------------------
class CustomTextEdit : public QTextEdit {
public:
    explicit CustomTextEdit(QWidget *parent = nullptr)
        : QTextEdit(parent),
          defaultFontSize(font().pointSizeF()),
          fileName("fun_edit.txt")
    {
        QString defaultText = "Welcome to the custom text editor!\n"
                              "Use Ctrl + Scroll to Zoom In/Out.\n"
                              "Press Ctrl + S to save.\n"
                              "Feel free to type and scroll around.\n"
                              "An OpenGL overlay will highlight lines & the cursor.";
        
        QFile file(fileName);
        if (file.exists()) {
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&file);
                setPlainText(in.readAll());
                file.close();
            } else {
                setPlainText(defaultText);
            }
        } else {
            setPlainText(defaultText);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&file);
                out << defaultText;
                file.close();
            }
        }
    }

    // Returns the current zoom factor relative to the default font size.
    float zoomFactor() const {
        return font().pointSizeF() / defaultFontSize;
    }

protected:
    void wheelEvent(QWheelEvent *event) override {
        if (event->modifiers() & Qt::ControlModifier) {
            int delta = event->angleDelta().y();
            (delta > 0) ? zoomInFont() : zoomOutFont();
        } else {
            QTextEdit::wheelEvent(event);
        }
    }

    void keyPressEvent(QKeyEvent *event) override {
        if ((event->modifiers() & Qt::ControlModifier) && event->key() == Qt::Key_S) {
            // Save the current text buffer to file.
            QFile file(fileName);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                qDebug() << "Failed to open" << fileName << "for writing.";
            } else {
                QTextStream out(&file);
                out << toPlainText();
                file.close();
                qDebug() << "Text saved to" << fileName;
            }
            event->accept();
        } else {
            QTextEdit::keyPressEvent(event);
        }
    }

private:
    float defaultFontSize;
    QString fileName;

    void zoomInFont() {
        QFont f = font();
        f.setPointSizeF(f.pointSizeF() + 1);
        setFont(f);
    }

    void zoomOutFont() {
        QFont f = font();
        float sz = f.pointSizeF();
        if (sz > 1) {
            f.setPointSizeF(sz - 1);
            setFont(f);
        }
    }
};

// -------------------- Helper Render Classes (Plain C++ Classes) --------------------

// CursorRenderer: Handles the cursor highlight shader.
class CursorRenderer {
public:
    void initialize() {
        static const char *vertexCursor = R"(
            #version 330 core
            layout(location = 0) in vec2 position;
            uniform float u_time;
            uniform vec2  u_offset;
            uniform float u_scale;
            void main() {
                float c = cos(u_time);
                float s = sin(u_time);
                mat2 rot = mat2(c, -s, s, c);
                vec2 pos = rot * (position * u_scale);
                gl_Position = vec4(pos + u_offset, 0.0, 1.0);
            }
        )";
        static const char *fragmentCursor = R"(
            #version 330 core
            out vec4 fragColor;
            void main() {
                fragColor = vec4(1.0, 0.0, 0.0, 0.5);
            }
        )";
        shaderProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexCursor);
        shaderProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentCursor);
        shaderProgram.link();
    }
    // f: pointer to current QOpenGLFunctions
    void draw(float time, const CustomTextEdit *textEdit, int widgetWidth, int widgetHeight, QOpenGLFunctions *f) {
        shaderProgram.bind();
        shaderProgram.setUniformValue("u_time", time);

        // Compute center of the cursor rectangle.
        QRect cRect = textEdit->cursorRect();
        QPoint center = cRect.center();
        float x = 2.0f * center.x() / float(widgetWidth) - 1.0f;
        float y = 1.0f - 2.0f * center.y() / float(widgetHeight);
        shaderProgram.setUniformValue("u_offset", QVector2D(x, y));
        shaderProgram.setUniformValue("u_scale", textEdit->zoomFactor());

        float verts[] = {
            -0.02f, -0.02f,
             0.02f, -0.02f,
             0.00f,  0.04f
        };
        f->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, verts);
        f->glEnableVertexAttribArray(0);
        f->glDrawArrays(GL_TRIANGLES, 0, 3);
        shaderProgram.release();
    }
private:
    QOpenGLShaderProgram shaderProgram;
};

// LineRenderer: Handles the line highlight shader.
class LineRenderer {
public:
    void initialize() {
        // Vertex shader now passes through vertex positions.
        static const char *vertexLine = R"(
            #version 330 core
            layout(location = 0) in vec2 position;
            uniform float u_time;
            uniform vec2  u_offset;
            out float v_y;
            void main() {
                v_y = position.y;
                float wave = 0.0;
                // Apply wave only for the top half
                wave = sin(u_time * 2.0 + position.x+0.01*position.y) * 0.03;
                gl_Position = vec4(position.x, position.y + wave + u_offset.y, 0.0, 1.0);
            }
        )";
        static const char *fragmentLine = R"(
            #version 330 core
            out vec4 fragColor;
            uniform float u_time;
            uniform float u_lineIndex;
            vec3 hue2rgb(float h) {
                h = fract(h);
                float r = abs(h * 6.0 - 3.0) - 1.0;
                float g = 2.0 - abs(h * 6.0 - 2.0);
                float b = 2.0 - abs(h * 6.0 - 4.0);
                return clamp(vec3(r, g, b), 0.0, 1.0);
            }
            void main() {
                float hue = fract(u_time * 0.1 + u_lineIndex * 0.1);
                vec3 color = hue2rgb(hue);
                fragColor = vec4(color, 0.3);
            }
        )";
        shaderProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexLine);
        shaderProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentLine);
        shaderProgram.link();
    }
    // f: pointer to current QOpenGLFunctions
    // Now draws a rectangle covering the line based on its actual height.
    void draw(float time, float lineIndex, const QRectF &lineRect, int widgetHeight, QOpenGLFunctions *f) {
        shaderProgram.bind();
        shaderProgram.setUniformValue("u_time", time);
        shaderProgram.setUniformValue("u_lineIndex", lineIndex);
        
        // Compute the center of the line in NDC:
        float centerY = 1.0f - 2.0f * ((lineRect.top() + lineRect.height()/2.0f) / float(widgetHeight));
        // Compute line height in NDC:
        float lineHeightNDC = 2.0f * (lineRect.height() / float(widgetHeight));
        float halfLine = lineHeightNDC / 2.0f;
        
        // Use u_offset to specify the center of the line highlight.
        shaderProgram.setUniformValue("u_offset", QVector2D(0.0f, centerY));

        // Create vertices for a rectangle covering the line.
        float verts[] = {
            -1.0f, -halfLine,
             1.0f, -halfLine,
            -1.0f,  halfLine,
             1.0f,  halfLine
        };
        f->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, verts);
        f->glEnableVertexAttribArray(0);
        f->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        shaderProgram.release();
    }
private:
    QOpenGLShaderProgram shaderProgram;
};

// -------------------- QT OpenGL integration --------------------
class OverlayWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT
public:
    explicit OverlayWidget(CustomTextEdit *edit, QWidget *parent = nullptr)
        : QOpenGLWidget(parent), textEdit(edit)
    {
        startTimer(16);
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_AlwaysStackOnTop);
    }

protected:
    void initializeGL() override {
        initializeOpenGLFunctions();
        cursorRenderer.initialize();
        lineRenderer.initialize();
    }
    void resizeGL(int w, int h) override {
        glViewport(0, 0, w, h);
    }
    void paintGL() override {
        glClearColor(0,0,0,0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        if (!textEdit) return;

        // Draw line highlights
        QTextBlock block = textEdit->document()->firstBlock();
        float lineIndex = 0.0f;
        while (block.isValid()) {
            QRectF lineRect = textEdit->document()->documentLayout()->blockBoundingRect(block);
            lineRenderer.draw(time, lineIndex, lineRect, height(), this);
            lineIndex += 1.0f;
            block = block.next();
        }
        // Draw the cursor highlight
        cursorRenderer.draw(time, textEdit, width(), height(), this);
    }
    void timerEvent(QTimerEvent *) override {
        time += 0.016f;
        update();
    }
private:
    CustomTextEdit *textEdit = nullptr;
    CursorRenderer  cursorRenderer;
    LineRenderer    lineRenderer;
    float           time = 0.0f;
};

// -------------------- Main App --------------------
class MyWindow : public QWidget {
    Q_OBJECT
public:
    explicit MyWindow(QWidget *parent = nullptr) : QWidget(parent)
    {
        QVBoxLayout *layout = new QVBoxLayout(this);
        container = new QWidget(this);
        container->setStyleSheet("background: white;");
        layout->addWidget(container);
        setLayout(layout);

        textEdit = new CustomTextEdit(container);
        textEdit->setGeometry(0, 0, container->width(), container->height());
        textEdit->show();

        overlay = new OverlayWidget(textEdit, container);
        overlay->setGeometry(textEdit->geometry());
        overlay->show();
        overlay->raise();

        container->installEventFilter(this);
    }
protected:
    bool eventFilter(QObject* obj, QEvent* e) override {
        if (obj == container && e->type() == QEvent::Resize) {
            QResizeEvent *re = static_cast<QResizeEvent*>(e);
            textEdit->setGeometry(0, 0, re->size().width(), re->size().height());
            overlay->setGeometry(textEdit->geometry());
            overlay->raise();
        }
        return QWidget::eventFilter(obj, e);
    }
private:
    QWidget        *container = nullptr;
    CustomTextEdit *textEdit  = nullptr;
    OverlayWidget  *overlay   = nullptr;
};

// -------------------- Single-file MOC --------------------
#include "main.moc"

// -------------------- main() --------------------
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
    window.setWindowTitle("Text Editor + Overlay (Modular Helpers, Fixed Lines)");
    window.show();

    return app.exec();
}

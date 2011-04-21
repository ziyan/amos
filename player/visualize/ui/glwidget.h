#ifndef AMOS_VISUALIZE_UI_GLWIDGET_H
#define AMOS_VISUALIZE_UI_GLWIDGET_H

#include <QtOpenGL>
#include <QThread>
#include <QMutex>

typedef struct vertex {
	float x, y, z;
} vertex_t;

typedef struct color {
	float r, g, b, a;
} color_t;

namespace amos
{
	class World;
	class Robot;
	class GLWidgetUpdater;

	/**
	 * Class GLWidget
	 * Main 3D world display
	 */
	class GLWidget : public QGLWidget {
		Q_OBJECT
	public:
		GLWidget(World *world, Robot *robot = 0, QWidget *parent = 0);
		virtual ~GLWidget();
		virtual void update();

	protected:
		/// GLWidget methods
		virtual void initializeGL();
		virtual void resizeGL(int w, int h);
		virtual void paintGL();

		/// 3D content drawing
		virtual void updateWorld();
		virtual void updateRobot();
		virtual void drawWorld();
		virtual void drawRobot();

		/// user interactions
		virtual void keyPressEvent(QKeyEvent*);
		virtual void keyReleaseEvent(QKeyEvent*);
		virtual void mousePressEvent(QMouseEvent*);
		virtual void mouseMoveEvent(QMouseEvent*);
		virtual void mouseReleaseEvent(QMouseEvent*);
		virtual void mouseDoubleClickEvent(QMouseEvent*);
		virtual void wheelEvent(QWheelEvent*);

		float color_map[201][3]; // cached color map

		// opengl stuff
		int width, height; // GLWidget viewport size
		bool vbo_supported;
		unsigned int vbo_vertices, vbo_colors;

		unsigned int count;
		vertex_t *vertices;
		color_t *colors;
		QMutex mutex;

		World *world;
		Robot *robot;
		float offset_x, offset_y;

		// user interaction related
		double lookat_x, lookat_y, eye_yaw, eye_pitch, eye_r; // viewing angle
		double lookat_x_old, lookat_y_old, eye_yaw_old, eye_pitch_old; // copy of viewing angle
		bool lookat_robot;
		int mouse_mode, mouse_x, mouse_y; // mouse state
		int mode;

		GLWidgetUpdater *updater;
	};

	class GLWidgetUpdater : public QThread
	{
		Q_OBJECT
	public:
		GLWidgetUpdater(GLWidget *parent) : QThread(), glwidget(parent) {
			connect(QApplication::instance(), SIGNAL(aboutToQuit()), this, SLOT(quit()));
			this->moveToThread(this);
			this->start();
		}
		virtual ~GLWidgetUpdater() { wait(); }
	protected Q_SLOTS:
		virtual void update() {
			glwidget->update();
		}
	protected:
		virtual void run() {
			QTimer timer(this);
			connect(&timer, SIGNAL(timeout()), this, SLOT(update()));
			timer.start(500);
			exec();
		}
		GLWidget *glwidget;
	};

}

#endif // AMOS_VISUALIZE_UI_GLWIDGET_H


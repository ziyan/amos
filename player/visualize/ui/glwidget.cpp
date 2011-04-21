#define GL_GLEXT_PROTOTYPES

#include "glwidget.h"
#include "glinfo.h"
#include "../model/world.h"
#include "../model/robot.h"
#include <libplayerc++/playerc++.h>
#include <QDebug>

#define MOUSE_MODE_NONE 0
#define MOUSE_MODE_PITCH_YAW 1
#define MOUSE_MODE_X_Y 2

#define MODE_E_AVG		0
#define MODE_E_VAR		1
#define MODE_RGB		2
#define MODE_P			3
#define MODE_P_CSPACE	4

using namespace amos;

GLWidget::GLWidget(World *world, Robot *robot, QWidget *parent) :
	QGLWidget(QGLFormat(QGL::DoubleBuffer | QGL::DepthBuffer | QGL::Rgba | QGL::AlphaChannel | QGL::SampleBuffers | QGL::DirectRendering), parent),
	vbo_supported(false), vbo_vertices(0), vbo_colors(0),
	count(0), vertices(0), colors(0),
	world(world), robot(robot),
	lookat_x(0), lookat_y(0), eye_yaw(-M_PI), eye_pitch(0.7), eye_r(3.0),
	lookat_x_old(0), lookat_y_old(0), eye_yaw_old(0.0), eye_pitch_old(0.0),
	lookat_robot(robot != 0),
	mouse_mode(MOUSE_MODE_NONE), mouse_x(0), mouse_y(0),
	mode(MODE_P)
{
	// change window title text
	window()->setWindowTitle("Visualization for Player");

	// tracking mouse and keyboard
	setMouseTracking(true);

	// color map
	for(int i = 0; i <= 200; i++)
	{
		QColor c = QColor::fromHsvF((float)i / 300.0, 0.9, 1.0);
		double r, g, b;
		c.getRgbF(&r, &g, &b);
		color_map[i][0] = r;
		color_map[i][1] = g;
		color_map[i][2] = b;
	}

	// calculate initial eye position
	if (world->getMap())
	{
		int x_min = 0, x_max = 0, y_min = 0, y_max = 0;
		std::set<map_tile_id_t> tiles = world->getMap()->list(true);
		for (std::set<map_tile_id_t>::iterator i = tiles.begin(); i != tiles.end(); ++i)
		{
			if (x_min == 0 && x_max == 0 && y_min == 0 && y_max == 0)
			{
				x_min = i->x; x_max = i->x;
				y_min = i->y; y_max = i->y;
			}
			else
			{
				if (i->x > x_max) x_max = i->x;
				if (i->x < x_min) x_min = i->x;
				if (i->y > y_max) y_max = i->y;
				if (i->y < y_min) y_min = i->y;
			}
		}
		offset_x = -world->getMap()->getInfo().scale * world->getMap()->getInfo().tile_width * ((x_min + x_max) / 2);
		offset_y = -world->getMap()->getInfo().scale * world->getMap()->getInfo().tile_height * ((y_min + y_max) / 2);
		qDebug() << "found" << tiles.size() << "tiles, bound is (" << x_min << y_min << ") to (" << x_max << y_max << ") offest (" << offset_x << offset_y << ")." << endl;
	}

	// frame updater
	QTimer *timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(updateGL()));
	timer->start(50);

	updater = new GLWidgetUpdater(this);
}

GLWidget::~GLWidget()
{
	if (vbo_vertices)
		glDeleteBuffersARB(1, &vbo_vertices);
	if (vbo_colors)
		glDeleteBuffersARB(1, &vbo_colors);
	if (vertices)
	{
		delete [] vertices;
		vertices = 0;
	}
	if (colors)
	{
		delete [] colors;
		colors = 0;
	}
	delete updater;
}


/// GLWidget methods

void GLWidget::initializeGL()
{
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_MULTISAMPLE);

	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

	// glinfo stuff
	gl_info_t info;
	if (!gl_get_info(info)) return;
	vbo_supported = gl_is_extension_supported(info, "GL_ARB_vertex_buffer_object");

	if (vbo_supported)
	{
		// create buffers
		glGenBuffersARB(1, &vbo_vertices);
		glGenBuffersARB(1, &vbo_colors);
		qDebug() << "VBO is supported.";
	}
}

void GLWidget::resizeGL(int w, int h)
{
	width = w;
	height = h;
	glViewport(0, 0, (GLint)w, (GLint)h);
}

void GLWidget::paintGL()
{
	static double lookat_robot_yaw = 0.0;
	if (robot)
	{
		if (lookat_robot)
		{
			lookat_x = offset_x + robot->getPosition2d()->GetXPos();
			lookat_y = offset_y + robot->getPosition2d()->GetYPos();
			eye_yaw += robot->getPosition2d()->GetYaw() - lookat_robot_yaw;
		}
		lookat_robot_yaw = robot->getPosition2d()->GetYaw();
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.0, (GLdouble)width/(GLdouble)height, 0.001, 4000.0);
	gluLookAt(lookat_x + eye_r * cos(eye_pitch) * cos(eye_yaw),
		lookat_y + eye_r * cos(eye_pitch) * sin(eye_yaw),
		0.5 + sin(eye_pitch) * eye_r,
		lookat_x, lookat_y, 0.5, -1.0 * cos(eye_yaw), -1.0 * sin(eye_yaw), 1.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	drawWorld();
	drawRobot();
}

void GLWidget::update()
{
	updateWorld();
	updateRobot();
}

/// 3D content drawing functions
void GLWidget::updateWorld()
{
	std::vector<vertex_t> vertices;
	std::vector<color_t> colors;
	const map_data_t *e_avg = 0, *e_var = 0, *r = 0, *g = 0, *b = 0, *p = 0, *p_cspace = 0;
	float tile_base_x = 0.0, tile_base_y = 0.0, cell_base_x = 0.0, cell_base_y = 0.0;
	float z = 0.0;
	uint32_t index = 0;
	color_t color = {0.0f, 0.0f, 0.0f, 0.0f}, color_bg = {0.0f, 0.0f, 0.0f, 0.0f};
	int mode = this->mode;
	float vx = lookat_x, vy = lookat_y, vr = eye_r;

	// cache
	Map *map = world->getMap();
	if (!map) return;
	map->refresh();
	map_info_t info = map->getInfo();

	// first, get a list of tiles
	std::set<map_tile_id_t> tiles = map->list(true);
	for (std::set<map_tile_id_t>::iterator i = tiles.begin(); i != tiles.end(); i++)
	{
		tile_base_x = offset_x + info.scale * (int)i->x * info.tile_width;
		tile_base_y = offset_y + info.scale * (int)i->y * info.tile_height;
		if (vr < 5.0 && sqrt((tile_base_x - vx) * (tile_base_x - vx) + (tile_base_y - vy) * (tile_base_y - vy)) > 100.0f) continue;

		if (mode == MODE_P)
		{
			if (i->channel != MAP_CHANNEL_P) continue;
			p = map->get((map_tile_id_t){MAP_CHANNEL_P, i->x, i->y, 0});
			if (!p) continue;
		}
		else if (mode == MODE_P_CSPACE)
		{
			if (i->channel != MAP_CHANNEL_P_CSPACE) continue;
			p_cspace = map->get((map_tile_id_t){MAP_CHANNEL_P_CSPACE, i->x, i->y, 0});
			if (!p_cspace) continue;
		}
		else
		{
			if (i->channel != MAP_CHANNEL_E_AVG) continue;
			// if we don't have data for the tile yet, we continue
			e_avg = map->get((map_tile_id_t){MAP_CHANNEL_E_AVG, i->x, i->y, 0});
			if (!e_avg) continue;
			e_var = map->get((map_tile_id_t){MAP_CHANNEL_E_VAR, i->x, i->y, 0});
			if (!e_var) continue;

			r = (mode == MODE_RGB) ? map->get((map_tile_id_t){MAP_CHANNEL_R, i->x, i->y, 0}) : 0;
			g = (mode == MODE_RGB) ? map->get((map_tile_id_t){MAP_CHANNEL_G, i->x, i->y, 0}) : 0;
			b = (mode == MODE_RGB) ? map->get((map_tile_id_t){MAP_CHANNEL_B, i->x, i->y, 0}) : 0;
		}



		// then build elevation mapping
		for (uint32_t y = 0; y < info.tile_height; ++y)
		{
			cell_base_y = tile_base_y + info.scale * y;

			for (uint32_t x = 0; x < info.tile_width; ++x)
			{
				cell_base_x = tile_base_x + info.scale * x;
				if (vr < 5.0 && sqrt((cell_base_x - vx) * (cell_base_x - vx) + (cell_base_y - vy) * (cell_base_y - vy)) > 15.0f) continue;
				index = y * info.tile_width + (int)x;

				if (mode == MODE_P)
				{
					z = p[index];
					if (z == 0.5f) continue;
					if (z > MAP_P_MAX) z = MAP_P_MAX;
					if (z < MAP_P_MIN) z = MAP_P_MIN;
					z = (z - 0.5f) * 0.4f;
				}
				else if (mode == MODE_P_CSPACE)
				{
					z = p_cspace[index];
					if (z == 0.5f) continue;
					if (z > MAP_P_MAX) z = MAP_P_MAX;
					if (z < MAP_P_MIN) z = MAP_P_MIN;
					z = (z - 0.5f) * 0.4f;
				}
				else
				{
					if (e_avg[index] == 0.0f && e_var[index] == 0.0f) continue;
					z = e_avg[index];
				}

				// draw a box
				vertices.push_back((vertex_t){cell_base_x, 				cell_base_y + info.scale,	z});
				vertices.push_back((vertex_t){cell_base_x + info.scale,	cell_base_y + info.scale,	z});
				vertices.push_back((vertex_t){cell_base_x + info.scale,	cell_base_y,				z});
				vertices.push_back((vertex_t){cell_base_x,				cell_base_y,				z});

				if (vr < 5.0)
				{
					vertices.push_back((vertex_t){cell_base_x, 				cell_base_y + info.scale,	z});
					vertices.push_back((vertex_t){cell_base_x + info.scale,	cell_base_y + info.scale,	z});
					vertices.push_back((vertex_t){cell_base_x + info.scale,	cell_base_y + info.scale,	-0.5f});
					vertices.push_back((vertex_t){cell_base_x,				cell_base_y + info.scale,	-0.5f});

					vertices.push_back((vertex_t){cell_base_x + info.scale, cell_base_y + info.scale,	z});
					vertices.push_back((vertex_t){cell_base_x + info.scale,	cell_base_y,				z});
					vertices.push_back((vertex_t){cell_base_x + info.scale,	cell_base_y,				-0.5f});
					vertices.push_back((vertex_t){cell_base_x + info.scale,	cell_base_y + info.scale,	-0.5f});

					vertices.push_back((vertex_t){cell_base_x + info.scale,	cell_base_y,				z});
					vertices.push_back((vertex_t){cell_base_x,				cell_base_y,				z});
					vertices.push_back((vertex_t){cell_base_x,				cell_base_y,				-0.5f});
					vertices.push_back((vertex_t){cell_base_x + info.scale,	cell_base_y,				-0.5f});

					vertices.push_back((vertex_t){cell_base_x,				cell_base_y,				z});
					vertices.push_back((vertex_t){cell_base_x,				cell_base_y + info.scale,	z});
					vertices.push_back((vertex_t){cell_base_x,				cell_base_y + info.scale,	-0.5f});
					vertices.push_back((vertex_t){cell_base_x,				cell_base_y,				-0.5f});
				}

				// determine color
				if (mode == MODE_RGB)
				{
					color = (color_t){r ? r[index] : 0.0f, g ? g[index] : 0.0f, b ? b[index] : 0.0f, 1.0f};
				}
				else if (mode == MODE_P)
				{
					float prob = p[index];
					if (prob > 1.0f) prob = 1.0f;
					if (prob < 0.0f) prob = 0.0f;
					color = (color_t){color_map[(int)(prob * 200)][0], color_map[(int)(prob * 200)][1], color_map[(int)(prob * 200)][2], 1.0f};
				}
				else if (mode == MODE_P_CSPACE)
				{
					float prob = p_cspace[index];
					if (prob > 1.0f) prob = 1.0f;
					if (prob < 0.0f) prob = 0.0f;
					color = (color_t){color_map[(int)(prob * 200)][0], color_map[(int)(prob * 200)][1], color_map[(int)(prob * 200)][2], 1.0f};
				}
				else if (mode == MODE_E_VAR)
				{
					float var = e_var[index] * 40000.0f;
					if (var > 1.0f) var = 1.0f;
					if (var < 0.0f) var = 0.0f;
					color = (color_t){color_map[(int)(var * 200)][0], color_map[(int)(var * 200)][1], color_map[(int)(var * 200)][2], 1.0f};
				}
				else
				{
					float avg = e_avg[index] + 0.5f;
					if (avg > 1.0f) avg = 1.0f;
					if (avg < 0.0f) avg = 0.0f;
					color = (color_t){color_map[(int)(avg * 200)][0], color_map[(int)(avg * 200)][1], color_map[(int)(avg * 200)][2], 1.0f};
				}
				colors.push_back(color); colors.push_back(color); colors.push_back(color); colors.push_back(color);
				if (vr < 5.0)
				{
					colors.push_back(color); colors.push_back(color); colors.push_back(color_bg); colors.push_back(color_bg);
					colors.push_back(color); colors.push_back(color); colors.push_back(color_bg); colors.push_back(color_bg);
					colors.push_back(color); colors.push_back(color); colors.push_back(color_bg); colors.push_back(color_bg);
					colors.push_back(color); colors.push_back(color); colors.push_back(color_bg); colors.push_back(color_bg);
				}
			}
		}
	}

	// convert to array format
	uint32_t count_new = vertices.size();
	Q_ASSERT(colors.size() == count_new);
	vertex_t *vertices_new = new vertex_t[count_new];
	color_t *colors_new = new color_t[count_new];

	uint32_t j = 0;
	for(std::vector<vertex_t>::iterator i = vertices.begin(); i != vertices.end(); ++i, ++j)
		vertices_new[j] = *i;
	j = 0;
	for(std::vector<color_t>::iterator i = colors.begin(); i != colors.end(); ++i, ++j)
		colors_new[j] = *i;

	mutex.lock();
	this->count = count_new;
	if (this->vertices) delete [] this->vertices;
	this->vertices = vertices_new;
	if (this->colors) delete [] this->colors;
	this->colors = colors_new;
	mutex.unlock();
}

void GLWidget::updateRobot()
{
}

void GLWidget::drawWorld()
{
	mutex.lock();

	if (vbo_supported)
	{
		if (this->vertices)
		{
			glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_vertices);
			glBufferDataARB(GL_ARRAY_BUFFER_ARB, count * sizeof(vertex_t), this->vertices, GL_STATIC_DRAW_ARB);
			delete [] this->vertices;
			this->vertices = 0;
		}
		if (this->colors)
		{
			glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_colors);
			glBufferDataARB(GL_ARRAY_BUFFER_ARB, count * sizeof(color_t), this->colors, GL_STATIC_DRAW_ARB);
			delete [] this->colors;
			this->colors = 0;
		}
	}

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	if(vbo_supported)
	{
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_vertices);
		glVertexPointer(3, GL_FLOAT, 0, 0);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_colors);
		glColorPointer(4, GL_FLOAT, 0, 0);
	}
	else
	{
		glVertexPointer(3, GL_FLOAT, 0, vertices);
		glColorPointer(4, GL_FLOAT, 0, colors);
	}

	glDrawArrays(GL_QUADS, 0, count);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);

	mutex.unlock();
}

void GLWidget::drawRobot()
{
	if (!robot) return;
	robot->getMutex()->lock();
	double robot_x = robot->getX();
	double robot_y = robot->getY();
	double robot_yaw = robot->getYaw();
	std::vector<player_pose2d_t> robot_path = robot->getPath();
	robot->getMutex()->unlock();

	glPushMatrix();
	glTranslatef(robot_x + offset_x, robot_y + offset_y, 0.0);
	glRotatef(robot_yaw / M_PI * 180.0, 0.0, 0.0, 1.0);
	glBegin(GL_TRIANGLES);
	glColor4d(1.0, 1.0, 1.0, 1.0);
	glVertex3d(0.25, 0.0, 0.25);
	glVertex3d(-0.25, 0.2, 0.25);
	glVertex3d(-0.25, -0.2, 0.25);
	glEnd();
	glBegin(GL_QUADS);
	glColor4d(1.0, 1.0, 1.0, 1.0);
	glVertex3d(0.25, 0.0, 0.25);
	glVertex3d(-0.25, 0.2, 0.25);
	glColor4d(0.0, 0.0, 0.0, 0.0);
	glVertex3d(-0.25, 0.2, 0.15);
	glVertex3d(0.25, 0.0, 0.15);

	glColor4d(1.0, 1.0, 1.0, 1.0);
	glVertex3d(-0.25, -0.2, 0.25);
	glVertex3d(0.25, 0.0, 0.25);
	glColor4d(0.0, 0.0, 0.0, 0.0);
	glVertex3d(0.25, 0.0, 0.15);
	glVertex3d(-0.25, -0.2, 0.15);

	glColor4d(1.0, 1.0, 1.0, 1.0);
	glVertex3d(-0.25, 0.2, 0.25);
	glVertex3d(-0.25, -0.2, 0.25);
	glColor4d(0.0, 0.0, 0.0, 0.0);
	glVertex3d(-0.25, -0.2, 0.15);
	glVertex3d(-0.25, 0.2, 0.15);

	glEnd();
	glPopMatrix();

	if (robot_path.size() > 1)
	{
		glColor4d(1.0, 1.0, 1.0, 1.0);
		glLineWidth(1.0);
		glBegin(GL_LINES);
		player_pose2d_t last_pose2d = *robot_path.begin();

		for(std::vector<player_pose2d_t>::const_iterator i = robot_path.begin() + 1; i != robot_path.end(); i++)
		{
			player_pose2d_t pose2d = *i;
			glVertex3d(last_pose2d.px + offset_x, last_pose2d.py + offset_y, 0.25);
			glVertex3d(pose2d.px + offset_x, pose2d.py + offset_y, 0.25);
			last_pose2d = pose2d;
		}
		glEnd();
	}
}



/// The following section handles user interaction

void GLWidget::keyPressEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Escape)
	{
		QApplication::quit();
	}
	else if (event->key() == Qt::Key_1)
	{
		mode = MODE_E_AVG;
	}
	else if (event->key() == Qt::Key_2)
	{
		mode = MODE_E_VAR;
	}
	else if (event->key() == Qt::Key_3)
	{
		mode = MODE_RGB;
	}
	else if (event->key() == Qt::Key_4)
	{
		mode = MODE_P;
	}
	else if (event->key() == Qt::Key_5)
	{
		mode = MODE_P_CSPACE;
	}
}

void GLWidget::keyReleaseEvent(QKeyEvent*)
{
}

void GLWidget::mousePressEvent(QMouseEvent *event)
{
	if(event->button() == Qt::LeftButton)
		mouse_mode = MOUSE_MODE_X_Y;
	if(event->button() == Qt::RightButton)
		mouse_mode = MOUSE_MODE_PITCH_YAW;
	mouse_x = event->x();
	mouse_y = event->y();
	eye_pitch_old = eye_pitch;
	eye_yaw_old = eye_yaw;
	lookat_x_old = lookat_x;
	lookat_y_old = lookat_y;
}
void GLWidget::mouseMoveEvent(QMouseEvent *event)
{
	if(mouse_mode == MOUSE_MODE_NONE) return;
	int dx = event->x() - mouse_x;
	int dy = event->y() - mouse_y;
	if(mouse_mode == MOUSE_MODE_X_Y) {
		double d = sqrt(((double)dx / 100.0)*((double)dx / 100.0) + ((double)dy / 100.0)*((double)dy / 100.0)) * eye_r / 5.0;
		double a = atan2(((double)dy / 100.0), ((double)dx / 100.0));
		lookat_x = lookat_x_old + sin(eye_yaw - a) * d;
		lookat_y = lookat_y_old - cos(eye_yaw - a) * d;
	}
	if(mouse_mode == MOUSE_MODE_PITCH_YAW) {
		eye_pitch = eye_pitch_old + (double)dy / 800.0;
		eye_yaw = eye_yaw_old - (double)dx / 500.0;
		if(eye_pitch < 0.0) eye_pitch = 0.0;
		if(eye_pitch > M_PI / 2.0) eye_pitch = M_PI / 2.0;
		if(eye_yaw < -M_PI) eye_yaw += M_PI + M_PI;
		if(eye_yaw > M_PI) eye_yaw -= M_PI + M_PI;
	}
}
void GLWidget::mouseReleaseEvent(QMouseEvent*)
{
	mouse_mode = MOUSE_MODE_NONE;
}

void GLWidget::mouseDoubleClickEvent(QMouseEvent*)
{
	lookat_robot = !lookat_robot;
}

void GLWidget::wheelEvent(QWheelEvent *event)
{
	for(int i = 0; i < abs(event->delta()); i++)
		eye_r *= (event->delta() < 0) ? 1.001 : 1.0 / 1.001;
}



#ifndef AMOS_VISUALIZE_MODEL_ROBOT_H
#define AMOS_VISUALIZE_MODEL_ROBOT_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <vector>
#include <libplayerc++/playerc++.h>

namespace amos
{
	/**
	 * Class Robot
	 * Connects to player server and update map
	 */
	class Robot : public QThread
	{
		Q_OBJECT
	public:
		/**
		 * Constructor for Robot
		 */
		Robot(const char* hostname = "localhost",
			const int port = 6665,
			const int position2d_index = 0,
			const int planner_index = -1);
		virtual ~Robot();

		/// data accessors
		PlayerCc::Position2dProxy *getPosition2d() { return position2d; }
		PlayerCc::PlannerProxy *getPlanner() { return planner; }
		double getX() { return x; }
		double getY() { return y; }
		double getYaw() { return yaw; }
		std::vector<player_pose2d_t> getPath() { return path; }
		QMutex *getMutex() { return &mutex; }

	protected Q_SLOTS:
		virtual void update();

	protected:
		virtual void run();

		PlayerCc::PlayerClient *client;
		PlayerCc::Position2dProxy *position2d;
		PlayerCc::PlannerProxy *planner;
		double x, y, yaw;
		std::vector<player_pose2d_t> path;
		QMutex mutex;
	};
}

#endif //AMOS_VISUALIZE_MODEL_ROBOT_H

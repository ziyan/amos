#ifndef AMOS_VISUALIZE_MODEL_WORLD_H
#define AMOS_VISUALIZE_MODEL_WORLD_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include "map/map.h"

namespace amos
{
	class GLWidget;
	/**
	 * Class World
	 * At the moment, this class does not do much, simply a container
	 */
	class World : public QThread {
		Q_OBJECT
	public:
		World(const std::vector< std::pair<std::string, uint16_t> > &servers);
		virtual ~World();
		Map *getMap() { return map; }

	protected Q_SLOTS:
		virtual void update();

	protected:
		virtual void run();

		Map *map;
	};
}

#endif //AMOS_VISUALIZE_MODEL_WORLD_H

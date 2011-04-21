#ifndef AMOS_PLUGINS_CSPACE_H
#define AMOS_PLUGINS_CSPACE_H

#include <libplayercore/playercore.h>
#include <vector>
#include <map>

#include "map/map.h"

namespace amos
{
	class CSpaceDriver : public ThreadedDriver
	{
	public:
		CSpaceDriver(ConfigFile*, int);
		virtual ~CSpaceDriver();
		virtual int MainSetup();
		virtual void MainQuit();
		virtual int ProcessMessage(QueuePointer&, player_msghdr*, void*);

	protected:
		virtual void Main();
		virtual int ProcessTile(const map_tile_id_t &id);

		std::vector< std::pair<std::string, uint16_t> > map_servers;
		Map *map;

		double radius, buffer;
		std::map<map_tile_id_t, uint32_t> revisions;
	};
}

#endif // AMOS_PLUGINS_CSPACE_H


#include "mapper.h"

#include <ctime>
#include <unistd.h>
#include <cassert>

using namespace amos;

MapperDriver::MapperDriver(ConfigFile* cf, int section) :
	ThreadedDriver(cf, section),
	map(0),
	position2d_dev(0),
	elevation_laser_dev(0),
	visual_camera_dev(0),
	probability_laser_dev(0)
{
	// initialize data
	memset(&elevation_laser_previous, 0, sizeof(player_pose3d_t) * 361);

	virtual_laser.min_angle = DTOR(-90.0);
	virtual_laser.max_angle = DTOR(90.0);
	virtual_laser.resolution = DTOR(0.5);
	virtual_laser.max_range = 8.0f;
	virtual_laser.ranges_count = 361;
	virtual_laser.ranges = &virtual_laser_ranges[0];
	virtual_laser.intensity_count = 361;
	virtual_laser.intensity = &virtual_laser_intensity[0];
	virtual_laser.id = 0;

	// read settings
	int map_servers_count = cf->GetTupleCount(section, "maphosts");
	if (map_servers_count > 0)
	{
		assert(map_servers_count == cf->GetTupleCount(section, "mapports"));
		for (int i = 0; i < map_servers_count; i++)
		{
			map_servers.push_back(make_pair(
				std::string(cf->ReadTupleString(section, "maphosts", i, "localhost")),
				(uint16_t)cf->ReadTupleInt(section, "mapports", i, 11211)
			));
		}
	}
	else
	{
		// default map server
		map_servers.push_back(make_pair(std::string("localhost"), (uint16_t)11211));
	}
	
	elevation_laser_pose.px = cf->ReadTupleFloat(section, "elevationlaserpose", 0, 0.0f);
	elevation_laser_pose.py = cf->ReadTupleFloat(section, "elevationlaserpose", 1, 0.0f);
	elevation_laser_pose.pz = cf->ReadTupleFloat(section, "elevationlaserpose", 2, 0.47f);
	elevation_laser_pose.proll = cf->ReadTupleFloat(section, "elevationlaserpose", 3, 0.0f);
	elevation_laser_pose.ppitch = cf->ReadTupleFloat(section, "elevationlaserpose", 4, -0.211931053f);
	elevation_laser_pose.pyaw = cf->ReadTupleFloat(section, "elevationlaserpose", 5, 0.0f);
	
	visual_camera_pose.px = cf->ReadTupleFloat(section, "visualcamerapose", 0, 0.0f);
	visual_camera_pose.py = cf->ReadTupleFloat(section, "visualcamerapose", 1, 0.0f);
	visual_camera_pose.pz = cf->ReadTupleFloat(section, "visualcamerapose", 2, 1.0f);
	visual_camera_pose.proll = cf->ReadTupleFloat(section, "visualcamerapose", 3, 0.0f);
	visual_camera_pose.ppitch = cf->ReadTupleFloat(section, "visualcamerapose", 4, DTOR(-45.0f));
	visual_camera_pose.pyaw = cf->ReadTupleFloat(section, "visualcamerapose", 5, 0.0f);
	visual_camera_hfov = cf->ReadTupleAngle(section, "camerafov", 0, 45.0f);
	visual_camera_vfov = cf->ReadTupleAngle(section, "camerafov", 1, 45.0);;


	if (cf->ReadDeviceAddr(&position2d_addr, section, "requires", PLAYER_POSITION2D_CODE, -1, NULL))
	{
		PLAYER_ERROR("mapper: cannot find position2d device addr");
		this->SetError(-1);
		return;
	}
	
	// elevation laser
	if (!cf->ReadDeviceAddr(&elevation_laser_addr, section, "requires", PLAYER_LASER_CODE, -1, "elevation"))
	{
		PLAYER_WARN("mapper: found elevation laser addr");
		
		// visual camera depends on elevation laser
		if (!cf->ReadDeviceAddr(&visual_camera_addr, section, "requires", PLAYER_CAMERA_CODE, -1, "visual"))
			PLAYER_WARN("mapper: found visual camera addr");
	}

	// probability laser
	if (!cf->ReadDeviceAddr(&probability_laser_addr, section, "requires", PLAYER_LASER_CODE, -1, "probability"))
	{
		PLAYER_WARN("mapper: found probability laser addr");
	}
	
	if (elevation_laser_addr.interf == PLAYER_LASER_CODE &&
		probability_laser_addr.interf == PLAYER_LASER_CODE)
	{
		PLAYER_ERROR("mapper: cannot have both elevation laser and probability laser in the same instance of mapper");
		this->SetError(-1);
		return;
	}
	
	if (elevation_laser_addr.interf != PLAYER_LASER_CODE &&
		probability_laser_addr.interf != PLAYER_LASER_CODE)
	{
		PLAYER_ERROR("mapper: nothing to map");
		this->SetError(-1);
		return;
	}

	// virtual laser output	
	if (!cf->ReadDeviceAddr(&virtual_laser_addr, section, "provides", PLAYER_LASER_CODE, -1, "virtual"))
	{
		if (this->AddInterface(virtual_laser_addr))
		{
			PLAYER_ERROR("mapper: cannot add virtual laser interface");
			this->SetError(-1);
			return;
		}
	}

	// add a dummy interface
	player_devaddr_t dummy_opaque_addr;
	if (cf->ReadDeviceAddr(&dummy_opaque_addr, section, "provides", PLAYER_OPAQUE_CODE, -1, "dummy"))
	{
		PLAYER_ERROR("mapper: cannot find dummy opaque interface");
		this->SetError(-1);
		return;
	}

	if (this->AddInterface(dummy_opaque_addr))
	{
		PLAYER_ERROR("mapper: cannot add dummy opaque interface");
		this->SetError(-1);
		return;
	}

	PLAYER_MSG0(3,"mapper: initialized");
}

MapperDriver::~MapperDriver()
{
}

int MapperDriver::MainSetup() {
	PLAYER_MSG0(3,"mapper: setup started");
	
	// first create the map client
	map = new Map(map_servers);
	if (!map->isOpen())
	{
		delete map;
		map = 0;
		PLAYER_ERROR("mapper: unable to create map");
		return -1;
	}
	ready = false;
	
	// subscribe to position2d
	if (!(position2d_dev = deviceTable->GetDevice(position2d_addr)))
	{
		PLAYER_ERROR("mapper: unable to locate suitable position2d device");
		return -1;
	}
	else if (position2d_dev->Subscribe(this->InQueue))
	{
		PLAYER_ERROR("mapper: position2d device cannot be subscribed");
		position2d_dev = 0;
		return -1;
	}

	// optional elevation laser
	if (elevation_laser_addr.interf == PLAYER_LASER_CODE)
	{
		if (!(elevation_laser_dev = deviceTable->GetDevice(elevation_laser_addr)))
		{
			PLAYER_ERROR("mapper: unable to locate suitable elevation laser device");
			return -1;
		}
		else if (elevation_laser_dev->Subscribe(this->InQueue))
		{
			PLAYER_ERROR("mapper: elevation laser device not subscribed");
			elevation_laser_dev = 0;
			return -1;
		}
	}
	
	// optional visual camera
	if (visual_camera_addr.interf == PLAYER_CAMERA_CODE)
	{
		if (!(visual_camera_dev = deviceTable->GetDevice(visual_camera_addr)))
		{
			PLAYER_ERROR("mapper: unable to locate suitable visual camera device");
			return -1;
		}
		else if (visual_camera_dev->Subscribe(this->InQueue))
		{
			PLAYER_ERROR("mapper: visual camera device not subscribed");
			visual_camera_dev = 0;
			return -1;
		}
	}
	
	// optional probability laser
	if (probability_laser_addr.interf == PLAYER_LASER_CODE)
	{
		if (!(probability_laser_dev = deviceTable->GetDevice(probability_laser_addr)))
		{
			PLAYER_ERROR("mapper: unable to locate suitable probability laser device");
			return -1;
		}
		else if (probability_laser_dev->Subscribe(this->InQueue))
		{
			PLAYER_ERROR("mapper: probability laser device not subscribed");
			probability_laser_dev = 0;
			return -1;
		}
	}

	PLAYER_MSG0(3,"mapper: setup complete");
	return 0;
}

void MapperDriver::MainQuit()
{
	PLAYER_MSG0(3,"mapper: shutting down");
	
	if (map)
	{
		delete map;
		map = 0;
	}

	if (position2d_dev)
	{
		position2d_dev->Unsubscribe(this->InQueue);
		position2d_dev = 0;
	}

	if (elevation_laser_dev)
	{
		elevation_laser_dev->Unsubscribe(this->InQueue);
		elevation_laser_dev = 0;
	}
	
	if (visual_camera_dev)
	{
		visual_camera_dev->Unsubscribe(this->InQueue);
		visual_camera_dev = 0;
	}
	
	if (probability_laser_dev)
	{
		probability_laser_dev->Unsubscribe(this->InQueue);
		probability_laser_dev = 0;
	}

	PLAYER_MSG0(3,"mapper: shut down");
}


void MapperDriver::Main()
{
	PLAYER_MSG0(3,"mapper: thread started");

	time_t timestamp = time(NULL);

	for(;;)
	{
		this->TestCancel();
		this->ProcessMessages();

		// see if we need to commit the map changes
		// commiting every second
		if (time(NULL) > timestamp)
		{
			map->commit();
			timestamp = time(NULL);
			PLAYER_MSG0(9, "mapper: map committed");
		}
		//map->refresh();
		//usleep(5000);
	}
}

int MapperDriver::ProcessMessage(QueuePointer &resp_queue, player_msghdr *hdr, void *data)
{
	// received position2d update
	if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, PLAYER_POSITION2D_DATA_STATE, position2d_addr) && data)
	{
		position2d_data = *((player_position2d_data_t*)data);
		ready = true;
		return 0;
	}
	// received elevation laser scan
	else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, PLAYER_LASER_DATA_SCAN, elevation_laser_addr) && data)
	{
		if (!ready) return 0;
		player_laser_data_t *d = (player_laser_data_t*)data;
		if (d->ranges_count != 361)
		{
			PLAYER_WARN("mapper: elevation laser data format not supported.");
			return -1;
		}
		PLAYER_MSG0(9, "mapper: map elevation laser begin");
		this->MapElevation(d->ranges, d->max_range);
		PLAYER_MSG0(9, "mapper: map elevation laser end");
		
		// publish virtual laser data?
		if (virtual_laser_addr.interf == PLAYER_LASER_CODE)
		{
			virtual_laser.max_range = d->max_range;
			virtual_laser.id ++;
			this->Publish(virtual_laser_addr, PLAYER_MSGTYPE_DATA, PLAYER_LASER_DATA_SCAN, &virtual_laser);
		}
		return 0;
	}
	// received visual camera frame
	else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, PLAYER_CAMERA_DATA_STATE, visual_camera_addr) && data)
	{
		if (!ready) return 0;
		player_camera_data_t *d = (player_camera_data_t*)data;
		if (d->bpp != 24 ||
			d->format != PLAYER_CAMERA_FORMAT_RGB888 ||
			d->compression != PLAYER_CAMERA_COMPRESS_RAW ||
			d->image_count != d->width * d->height * 3)
		{
			PLAYER_WARN("mapper: visual camera image format not supported.");
			return -1;
		}
		PLAYER_MSG0(9, "mapper: map visual camera begin");
		this->MapVisual(d->image, d->width, d->height);
		PLAYER_MSG0(9, "mapper: map visual camera end");
		return 0;
	}
	// received probability laser scan
	else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, PLAYER_LASER_DATA_SCAN, probability_laser_addr) && data)
	{
		if (!ready) return 0;
		player_laser_data_t *d = (player_laser_data_t*)data;
		if (d->ranges_count != 361)
		{
			PLAYER_WARN("mapper: probability laser data format not supported.");
			return -1;
		}
		PLAYER_MSG0(9, "mapper: map probability laser begin");
		this->MapProbability(d->ranges, d->max_range);
		PLAYER_MSG0(9, "mapper: map probability laser end");
		return 0;
	}
	return -1;
}


Driver* driver_init(ConfigFile* cf, int section) {
	return new MapperDriver(cf, section);
}

void driver_register(DriverTable* table) {
	table->AddDriver("amosmapper", driver_init);
}

extern "C" {
	int player_driver_init(DriverTable* table) {
		driver_register(table);
		return 0;
	}
}


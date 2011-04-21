#include "laserfusion.h"

#include <unistd.h>

using namespace amos;

LaserFusionDriver::LaserFusionDriver(ConfigFile* cf, int section) :
	ThreadedDriver(cf, section)
{
	lasers_count = cf->GetTupleCount(section, "requires");
	assert(lasers_count > 0);
	
	lasers_addr = new player_devaddr_t[lasers_count];
	lasers_dev = new Device*[lasers_count];
	lasers_ranges = new float[lasers_count * 361];

	memset(lasers_addr, 0, lasers_count * sizeof(player_devaddr_t));
	memset(lasers_dev, 0, lasers_count * sizeof(Device*));
	memset(lasers_ranges, 0, lasers_count * 361 * sizeof(float));
	
	memset(laser_ranges, 0, 361 * sizeof(float));
	memset(laser_intensity, 0, 361 * sizeof(uint8_t));

	// set up a laser interface
	if (cf->ReadDeviceAddr(&laser_addr, section, "provides", PLAYER_LASER_CODE, -1, NULL))
	{
		PLAYER_ERROR("laserfusion: cannot find dummy opaque device addr");
		this->SetError(-1);
		return;
	}
	if (this->AddInterface(laser_addr))
	{
		PLAYER_ERROR("laserfusion: cannot add dummy opaque interface");
		this->SetError(-1);
		return;
	}
	
	for (int i = 0; i < lasers_count; i++)
	{
		if (cf->ReadDeviceAddr(&lasers_addr[i], section, "requires", PLAYER_LASER_CODE, i, NULL))
		{
			PLAYER_ERROR("laserfusion: cannot find address of required laser");
			this->SetError(-1);
			return;
		}
	}
	
	laser_data.min_angle = DTOR(-90.0);
	laser_data.max_angle = DTOR(90.0);
	laser_data.resolution = DTOR(0.5);
	laser_data.max_range = 8.0f;
	laser_data.ranges_count = 361;
	laser_data.ranges = &laser_ranges[0];
	laser_data.intensity_count = 361;
	laser_data.intensity = &laser_intensity[0];
	laser_data.id = 0;
	PLAYER_MSG0(8,"laserfusion: initialized");
}

LaserFusionDriver::~LaserFusionDriver() {
	if (lasers_addr)
	{
		delete [] lasers_addr;
		lasers_addr = 0;
	}
	if (lasers_dev)
	{
		delete [] lasers_dev;
		lasers_dev = 0;
	}
	if (lasers_ranges)
	{
		delete [] lasers_ranges;
		lasers_ranges = 0;
	}
}

int LaserFusionDriver::MainSetup() {
	PLAYER_MSG0(8,"laserfusion: setup begin");
	
	for (int i = 0; i < lasers_count; i++)
	{
		if(!(lasers_dev[i] = deviceTable->GetDevice(lasers_addr[i])))
		{
			PLAYER_ERROR("laserfusion: unable to locate suitable laser device");
			return -1;
		}
		if(lasers_dev[i]->Subscribe(InQueue) != 0)
		{
			PLAYER_ERROR("laserfusion: unable to subscribe to laser device");
			lasers_dev[i] = 0;
			return -1;
		}
	}
	PLAYER_MSG0(8,"laserfusion: setup completed");
	return 0;
}

void LaserFusionDriver::MainQuit() {
	PLAYER_MSG0(8,"laserfusion: unsubscribing");
	for (int i = 0; i < lasers_count; i++)
	{
		if(lasers_dev[i])
		{
			lasers_dev[i]->Unsubscribe(this->InQueue);
			lasers_dev[i] = 0;
		}
	}
	PLAYER_MSG0(8,"laserfusion: shutted down");
}


void LaserFusionDriver::Main()
{
	PLAYER_MSG0(8,"laserfusion: main thread started");
	for(;;) {
		this->TestCancel();
		this->ProcessMessages();
		usleep(10000);
	}
}

int LaserFusionDriver::ProcessMessage(QueuePointer &resp_queue, player_msghdr *hdr, void *data)
{
	for (int i = 0; i < lasers_count; i++)
	{
		if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, PLAYER_LASER_DATA_SCAN, lasers_addr[i]) && data)
		{
			player_laser_data_t *d = (player_laser_data_t*)data;
			if (d->ranges_count != 361)
			{
				PLAYER_WARN("mapper: laser data format not supported.");
				return -1;
			}
			memcpy(lasers_ranges + i * 361, d->ranges, 361 * sizeof(float));
			this->Fuse();
			return 0;
		}
	}
	return -1;
}

void LaserFusionDriver::Fuse()
{
	// reset laser to max range first
	for (int j = 0; j < 361; j++)
	{
		laser_ranges[j] = laser_data.max_range;
	}

	// take smallest laser range from each angle
	for (int i = 0; i < lasers_count; i++)
	{
		for (int j = 0; j < 361; j++)
		{
			if (laser_ranges[j] > lasers_ranges[i * 361 + j])
				laser_ranges[j] = lasers_ranges[i * 361 + j];
		}
	}
	laser_data.id ++;
	this->Publish(laser_addr, PLAYER_MSGTYPE_DATA, PLAYER_LASER_DATA_SCAN, &laser_data);
}


Driver* driver_init(ConfigFile* cf, int section) {
	return new LaserFusionDriver(cf, section);
}

void driver_register(DriverTable* table) {
	table->AddDriver("amoslaserfusion", driver_init);
}

extern "C" {
	int player_driver_init(DriverTable* table) {
		driver_register(table);
		return 0;
	}
}




#include "localize.h"

#include <cmath>
#include <unistd.h>

using namespace amos;

LocalizeDriver::LocalizeDriver(ConfigFile* cf, int section) :
	ThreadedDriver(cf, section),
	encoder_dev(0), gps_dev(0), compass_dev(0), md_dev(0)
{
	memset(&position2d_data, 0, sizeof(player_position2d_data_t));
	memset(&encoder_data, 0, sizeof(player_position2d_data_t));
	memset(&gps_data, 0, sizeof(player_gps_data_t));
	memset(&compass_data, 0, sizeof(player_position2d_data_t));
	memset(&md_data, 0, sizeof(player_position2d_data_t));

	if (cf->ReadDeviceAddr(&position2d_addr, section, "provides", PLAYER_POSITION2D_CODE, -1, NULL))
	{
		PLAYER_ERROR("localize: cannot find position addr");
		this->SetError(-1);
		return;
	}
	if (this->AddInterface(position2d_addr))
	{
		PLAYER_ERROR("localize: cannot add position interface");
		this->SetError(-1);
		return;
	}

	if (cf->ReadDeviceAddr(&encoder_addr, section, "requires", PLAYER_POSITION2D_CODE, -1, "encoder"))
	{
		PLAYER_ERROR("localize: cannot find drive addr");
		this->SetError(-1);
		return;
	}
	if (cf->ReadDeviceAddr(&gps_addr, section, "requires", PLAYER_GPS_CODE, -1, NULL))
		PLAYER_WARN("localize: gps will not be used");
	if (cf->ReadDeviceAddr(&compass_addr, section, "requires", PLAYER_POSITION2D_CODE, -1, "compass"))
		PLAYER_WARN("localize: compass will not be used");
	if (cf->ReadDeviceAddr(&md_addr, section, "requires", PLAYER_POSITION2D_CODE, -1, "md"))
		PLAYER_WARN("localize: md will not be used");

	PLAYER_MSG0(3,"localize: initialized");
}

LocalizeDriver::~LocalizeDriver()
{
}

int LocalizeDriver::MainSetup()
{
	PLAYER_MSG0(3,"localize: setup begin");
	if(!(encoder_dev = deviceTable->GetDevice(encoder_addr)))
	{
		PLAYER_ERROR("localize: unable to locate suitable drive device");
		return -1;
	}
	if(encoder_dev->Subscribe(InQueue))
	{
		PLAYER_ERROR("localize: unable to subscribe to drive device");
		encoder_dev = 0;
		return -1;
	}

	if (gps_addr.interf)
	{
		if(!(gps_dev = deviceTable->GetDevice(gps_addr)))
		{
			PLAYER_ERROR("localize: unable to locate suitable gps device");
			return -1;
		}
		if(gps_dev->Subscribe(InQueue) != 0)
		{
			PLAYER_WARN("localize: unable to subscribe to gps device, gps will not be used");
			gps_dev = 0;
		}
	}

	if (compass_addr.interf)
	{
		if(!(compass_dev = deviceTable->GetDevice(compass_addr)))
		{
			PLAYER_ERROR("localize: unable to locate suitable compass device");
			return -1;
		}
		if(compass_dev->Subscribe(InQueue))
		{
			PLAYER_ERROR("localize: unable to subscribe to compass device");
			compass_dev = 0;
			return -1;
		}
	}

	if (md_addr.interf)
	{
		if(!(md_dev = deviceTable->GetDevice(md_addr)))
		{
			PLAYER_ERROR("localize: unable to locate suitable md device");
			return -1;
		}
		if(md_dev->Subscribe(InQueue))
		{
			PLAYER_ERROR("localize: unable to subscribe to md device");
			compass_dev = 0;
			return -1;
		}
	}

	PLAYER_MSG0(3,"localize: setup completed");
	return 0;
}

void LocalizeDriver::MainQuit()
{
	PLAYER_MSG0(3,"localize: shutting down");
	if (encoder_dev)
	{
		encoder_dev->Unsubscribe(this->InQueue);
		encoder_dev = 0;
	}
	if (gps_dev)
	{
		gps_dev->Unsubscribe(this->InQueue);
		gps_dev = 0;
	}
	if (compass_dev)
	{
		compass_dev->Unsubscribe(this->InQueue);
		compass_dev = 0;
	}
	if (md_dev)
	{
		md_dev->Unsubscribe(this->InQueue);
		md_dev = 0;
	}
	PLAYER_MSG0(3,"localize: shut down");
}


void LocalizeDriver::Main()
{
	PLAYER_MSG0(3,"localize: thread started");
	for(;;)
	{
		this->TestCancel();
		this->ProcessMessages();
		usleep(10000);
	}
}


int LocalizeDriver::ProcessMessage(QueuePointer &resp_queue, player_msghdr *hdr, void *data)
{
	if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, PLAYER_GPS_DATA_STATE, gps_addr))
	{
		if (!data || hdr->size != sizeof(player_gps_data_t)) return -1;

		player_gps_data_t *d = (player_gps_data_t*)data;

		// check to see if the fix is fresh
		bool is_fresh = d->time_sec > gps_data.time_sec ||
						(d->time_sec == gps_data.time_sec && d->time_usec > gps_data.time_usec);
		gps_data = *((player_gps_data_t*)data);

		// we only accept fresh DGPS updates
		if (!is_fresh)
		{
			PLAYER_MSG0(9, "localizer: gps data is not fresh");
			return 0;
		}

		if (gps_data.quality < 2)
		{
			PLAYER_WARN("localizer: gps has no DGPS fix");
			return 0;
		}

		if (gps_data.hdop > 13)
		{
			PLAYER_WARN1("localizer: gps data is low quality, hdop = %f", (double)gps_data.hdop/10.0);
			return 0;
		}

		position2d_data.pos.px = gps_data.utm_e;
		position2d_data.pos.py = gps_data.utm_n;

		// publish location
		this->Publish(position2d_addr, PLAYER_MSGTYPE_DATA, PLAYER_POSITION2D_DATA_STATE, &position2d_data);

		// update motor odomerty
		player_position2d_set_odom_req_t req = { position2d_data.pos };
		Message *msg = encoder_dev->Request(this->InQueue, PLAYER_MSGTYPE_REQ, PLAYER_POSITION2D_REQ_SET_ODOM, &req, sizeof(player_position2d_set_odom_req_t), NULL);
		if (msg)
		{
			delete msg;
			msg = 0;
		}
		return 0;
	}
	else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, PLAYER_POSITION2D_DATA_STATE, compass_addr))
	{
		if (!data || hdr->size != sizeof(player_position2d_data_t)) return -1;

		compass_data = *((player_position2d_data_t*)data);
		position2d_data.pos.pa = compass_data.pos.pa + md_data.vel.pa;

		// publish location
		this->Publish(position2d_addr, PLAYER_MSGTYPE_DATA, PLAYER_POSITION2D_DATA_STATE, &position2d_data);

		// update motor odometry
		player_position2d_set_odom_req_t req = { position2d_data.pos };
		Message *msg = encoder_dev->Request(this->InQueue, PLAYER_MSGTYPE_REQ, PLAYER_POSITION2D_REQ_SET_ODOM, &req, sizeof(player_position2d_set_odom_req_t), NULL);
		if (msg)
		{
			delete msg;
			msg = 0;
		}
		return 0;
	}
	else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, PLAYER_POSITION2D_DATA_STATE, md_addr))
	{
		if (!data || hdr->size != sizeof(player_position2d_data_t)) return -1;

		// received magnetic declination data
		md_data = *((player_position2d_data_t*)data);
		return 0;
	}
	else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, PLAYER_POSITION2D_DATA_STATE, encoder_addr))
	{
		if (!data || hdr->size != sizeof(player_position2d_data_t)) return -1;

		encoder_data = *((player_position2d_data_t*)data);
		position2d_data = encoder_data;

		// publish location
		this->Publish(position2d_addr, PLAYER_MSGTYPE_DATA, PLAYER_POSITION2D_DATA_STATE, &position2d_data);
		return 0;
	}
	else if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD, -1, position2d_addr))
	{
		// pass thru commands
		player_msghdr_t newhdr = *hdr;
		newhdr.addr = position2d_addr;
		encoder_dev->PutMsg(this->InQueue, &newhdr, data);
		return 0;
	}
	else if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, -1, position2d_addr))
	{
		// pass thru requests
		Message* msg = encoder_dev->Request(this->InQueue, hdr->type, hdr->subtype, data, hdr->size, &hdr->timestamp);
		if (!msg) return -1;
		player_msghdr_t* rephdr = msg->GetHeader();
		void* repdata = msg->GetPayload();
		rephdr->addr = position2d_addr;
		this->Publish(resp_queue, rephdr, repdata);
		delete msg;
		return 0;
	}
	return -1;
}

Driver* driver_init(ConfigFile* cf, int section) {
	return new LocalizeDriver(cf, section);
}

void driver_register(DriverTable* table) {
	table->AddDriver("amoslocalize", driver_init);
}

extern "C" {
	int player_driver_init(DriverTable* table) {
		driver_register(table);
		return 0;
	}
}


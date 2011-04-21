#include "gps.h"
#include "serial/serial.h"
#include "nmea.h"

#include <time.h>
#include <fcntl.h>
#include <math.h>
#include <stdlib.h>

using namespace std;
using namespace amos;

GPSDriver::GPSDriver(ConfigFile* cf, int section) : ThreadedDriver(cf, section)
{
	port = cf->ReadString(section, "port", "/dev/gps");
	int baud = cf->ReadInt(section, "baudrate", 38400);
	baudrate = serial_baudrate(baud, B38400);
	PLAYER_MSG2(5,"gps: config: port = %s, baudrate = %i", port.c_str(), baud);


	if (cf->ReadDeviceAddr(&gps_addr, section, "provides", PLAYER_GPS_CODE, -1, NULL))
	{
		PLAYER_ERROR("gps: cannot find gps addr");
		this->SetError(-1);
		return;
	}
	if (this->AddInterface(gps_addr)) {
		PLAYER_ERROR("gps: cannot add gps interface");
		this->SetError(-1);
		return;
	}


	if (!cf->ReadDeviceAddr(&position2d_addr, section, "provides", PLAYER_POSITION2D_CODE, -1, NULL))
	{
		if (this->AddInterface(position2d_addr))
		{
			PLAYER_ERROR("gps: cannot add position2d interface");
			this->SetError(-1);
			return;
		}
	}
	PLAYER_MSG0(3,"gps: initialized");
}

GPSDriver::~GPSDriver()
{
}

int GPSDriver::MainSetup()
{
	PLAYER_MSG0(3,"gps: setup started");
	fd = serial_open(port.c_str(), baudrate);
	if(fd < 0)
	{
		PLAYER_ERROR1("gps: failed to connect to motor at %s", port.c_str());
		return -1;
	}

	PLAYER_MSG0(3,"gps: port openned, baudrate set");
	PLAYER_MSG0(3,"gps: setup complete");
	return 0;
}

void GPSDriver::MainQuit()
{
	PLAYER_MSG0(3,"gps: shutting down");
	serial_close(fd);
	fd = 0;
	PLAYER_MSG0(3,"gps: shutted down");
}


void GPSDriver::Main()
{
	PLAYER_MSG0(3,"gps: thread started");

	player_gps_data_t gps_data;
	player_position2d_data_t position2d_data;
	memset(&gps_data, 0, sizeof(player_gps_data_t));
	memset(&position2d_data, 0, sizeof(player_position2d_data_t));

	for(;;)
	{
		this->TestCancel();
		this->ProcessMessages();
		if (nmea(fd, &gps_data, &position2d_data))
		{
			PLAYER_WARN("gps: nmea read failed");
		}
		this->Publish(gps_addr, PLAYER_MSGTYPE_DATA, PLAYER_GPS_DATA_STATE, &gps_data);
		if (position2d_addr.interf)
			this->Publish(position2d_addr, PLAYER_MSGTYPE_DATA, PLAYER_POSITION2D_DATA_STATE, &position2d_data);
	}
}


int GPSDriver::ProcessMessage(QueuePointer &resp_queue, player_msghdr *hdr, void *data)
{
	return -1;
}

Driver* driver_init(ConfigFile* cf, int section) {
	return new GPSDriver(cf, section);
}

void driver_register(DriverTable* table) {
	table->AddDriver("amosgps", driver_init);
}

extern "C" {
	int player_driver_init(DriverTable* table) {
		driver_register(table);
		return 0;
	}
}




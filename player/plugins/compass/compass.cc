#include "compass.h"
#include "serial/serial.h"

#include <time.h>
#include <fcntl.h>
#include <math.h>
#include <stdlib.h>

#define SERIAL_TIMEOUT 1000000

using namespace std;
using namespace amos;

CompassDriver::CompassDriver(ConfigFile* cf, int section) : ThreadedDriver(cf, section) {
	port = cf->ReadString(section, "port", "/dev/ttyUSB1");
	int baud = cf->ReadInt(section, "baudrate", 9600);
	baudrate = serial_baudrate(baud, B9600);
	PLAYER_MSG2(5,"compass: config: port = %s, baudrate = %i", port.c_str(), baud);

	if (cf->ReadDeviceAddr(&position2d_addr, section, "provides", PLAYER_POSITION2D_CODE, -1, NULL) != 0) {
		PLAYER_ERROR("compass: cannot find compass addr");
		this->SetError(-1);
		return;
	}
	if (this->AddInterface(position2d_addr)) {
		PLAYER_ERROR("compass: cannot add compass interface");
		this->SetError(-1);
		return;
	}

	PLAYER_MSG0(3,"compass: initialized");
}

CompassDriver::~CompassDriver() {
}

int CompassDriver::MainSetup() {
	PLAYER_MSG0(3,"compass: setup started");
	fd = serial_open(port.c_str(), baudrate);
	if(fd < 0)
	{
		PLAYER_ERROR1("compass: failed to connect to motor at %s", port.c_str());
		return -1;
	}

	PLAYER_MSG0(3,"compass: port openned, baudrate set");
	PLAYER_MSG0(3,"compass: setup complete");
	return 0;
}

void CompassDriver::MainQuit() {
	PLAYER_MSG0(3,"compass: shutting down");
	serial_close(fd);
	fd = 0;
	PLAYER_MSG0(3,"compass: shutted down");
}


void CompassDriver::Main()
{
	PLAYER_MSG0(3,"compass: thread started");
	char buf[2];
	int heading;
	player_position2d_data_t compass_data = {{0}, {0}};

	sleep(5);

	for(;;)
	{
		this->TestCancel();
		this->ProcessMessages();

		if (serial_wait_mark(fd, 0x02, SERIAL_TIMEOUT))
		{
			PLAYER_WARN("compass: failed to read from the serial port");
			continue;
		}

		if (serial_timed_read(fd, buf, 2, SERIAL_TIMEOUT))
		{
			PLAYER_WARN("compass: failed to read from the serial port");
			continue;
		}

		//convert to radian
		heading = (((buf[0] << 8) & 0xff00) | (buf[1] & 0xff)) % 3600;
		compass_data.pos.pa = (float)(-heading + 900) / 1800.0f * M_PI;
		while(compass_data.pos.pa < -M_PI) compass_data.pos.pa += M_PI + M_PI;
		while(compass_data.pos.pa >= M_PI) compass_data.pos.pa -= M_PI + M_PI;

		PLAYER_MSG3(9,"compass: heading = %f (radian), %f (degree), %i (raw)", compass_data.pos.pa, compass_data.pos.pa/M_PI * 180.0, heading);

		this->Publish(this->position2d_addr, PLAYER_MSGTYPE_DATA, PLAYER_POSITION2D_DATA_STATE, &compass_data);

		usleep(10000);
	}
}


int CompassDriver::ProcessMessage(QueuePointer &resp_queue, player_msghdr *hdr, void *data) {
	return -1;
}

Driver* driver_init(ConfigFile* cf, int section) {
	return new CompassDriver(cf, section);
}

void driver_register(DriverTable* table) {
	table->AddDriver("amoscompass", driver_init);
}

extern "C" {
	int player_driver_init(DriverTable* table) {
		driver_register(table);
		return 0;
	}
}




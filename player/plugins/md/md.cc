#include "md.h"
#include "WMM.h"

#include <cmath>
#include <stdlib.h>

using namespace std;
using namespace amos;

MagneticDeclinationDriver::MagneticDeclinationDriver(ConfigFile* cf, int section) : ThreadedDriver(cf, section)
{
	memset(&position2d_data, 0, sizeof(player_position2d_data_t));

	// read default declination
	declination = cf->ReadFloat(section, "declination", 0.0f);
	filename = cf->ReadFilename(section, "file", "EGM9615.BIN");

	// optional gps
	if (cf->ReadDeviceAddr(&gps_addr, section, "requires", PLAYER_GPS_CODE, -1, NULL))
		PLAYER_WARN("mapper: cannot find gps device addr, gps will not be used");

	if (cf->ReadDeviceAddr(&position2d_addr, section, "provides", PLAYER_POSITION2D_CODE, -1, NULL) != 0)
	{
		PLAYER_ERROR("md: cannot find position2d addr");
		this->SetError(-1);
		return;
	}
	if (this->AddInterface(position2d_addr))
	{
		PLAYER_ERROR("md: cannot add position2d interface");
		this->SetError(-1);
		return;
	}

	PLAYER_MSG0(3,"md: initialized");
}

MagneticDeclinationDriver::~MagneticDeclinationDriver()
{
}

int MagneticDeclinationDriver::MainSetup()
{
	PLAYER_MSG0(3,"md: setup started");

	if (gps_addr.interf)
	{
		if(!(gps_dev = deviceTable->GetDevice(gps_addr)))
		{
			PLAYER_ERROR("md: unable to locate suitable gps device");
			return -1;
		}
		if(gps_dev->Subscribe(InQueue) != 0)
		{
			PLAYER_WARN("md: unable to subscribe to gps device, gps will not be used");
			gps_dev = 0;
		}
	}

	PLAYER_MSG0(3,"md: setup complete");
	return 0;
}

void MagneticDeclinationDriver::MainQuit()
{
	PLAYER_MSG0(3,"md: shutting down");
	if (gps_dev)
	{
		gps_dev->Unsubscribe(this->InQueue);
		gps_dev = 0;
	}
	PLAYER_MSG0(3,"md: shutted down");
}


void MagneticDeclinationDriver::Main()
{
	PLAYER_MSG0(8,"md: thread started");
	for(;;)
	{
		this->TestCancel();
		this->ProcessMessages();

		position2d_data.vel.pa = -DTOR(declination);
		this->Publish(position2d_addr, PLAYER_MSGTYPE_DATA, PLAYER_POSITION2D_DATA_STATE, &position2d_data);
		usleep(100000);
	}
}


int MagneticDeclinationDriver::ProcessMessage(QueuePointer &resp_queue, player_msghdr *hdr, void *data) {
	// gps
	if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, PLAYER_GPS_DATA_STATE, gps_addr))
	{
		if (!data || hdr->size != sizeof(player_gps_data_t)) return -1;
		player_gps_data_t* d = (player_gps_data_t*)data;
		if(d->quality > 0)
		{
			double latitude = ((double)d->latitude)/1e7;
			double longitude = ((double)d->longitude)/1e7;
			double elevation = ((double)d->altitude)/1e3;
			PLAYER_MSG3(8,"md: received gps data latitude = %f, longitude = %f, elevation = %f", latitude, longitude, elevation);
			if (UpdateDeclination(latitude, longitude, elevation))
			{
				PLAYER_WARN1("md: magnetic declination = %f", declination);
				PLAYER_MSG0(3,"md: unsubscribing gps");
				if (gps_dev)
				{
					gps_dev->Unsubscribe(this->InQueue);
					gps_dev = 0;
				}
			}
		}
		return 0;
	}
	return -1;
}


bool MagneticDeclinationDriver::UpdateDeclination(double latitude, double longitude, double elevation)
{
	bool ret = false;

	WMMtype_MagneticModel *MagneticModel = 0, *TimedMagneticModel = 0;
	WMMtype_Ellipsoid Ellip = {0};
	WMMtype_CoordSpherical CoordSpherical = {0};
	WMMtype_CoordGeodetic CoordGeodetic = {0};
	WMMtype_Date UserDate = {0};
	WMMtype_GeoMagneticElements GeoMagneticElements = {0};
	WMMtype_Geoid Geoid = {0};
	char Error_Message[255] = {0};
	int NumTerms = 0;
	time_t t = time(NULL);
	struct tm *ltime = localtime(&t);

	/* Memory allocation */
	NumTerms = ( ( WMM_MAX_MODEL_DEGREES + 1 ) * ( WMM_MAX_MODEL_DEGREES + 2 ) / 2 );    /* WMM_MAX_MODEL_DEGREES is defined in WMM_Header.h */

	MagneticModel = WMM_AllocateModelMemory(NumTerms);  /* For storing the WMM Model parameters */
	TimedMagneticModel = WMM_AllocateModelMemory(NumTerms);  /* For storing the time modified WMM Model parameters */

	if(MagneticModel == NULL || TimedMagneticModel == NULL)
	{
		WMM_Error(2);
		goto exit;
	}

	WMM_SetDefaults(&Ellip, MagneticModel, &Geoid); /* Set default values and constants */

	/* Check for Geographic Poles */
	WMM_load2010MagneticModel(MagneticModel);

	if (!WMM_InitializeGeoid(&Geoid, (char*)filename.c_str())) /* Read the Geoid file */
	{
		PLAYER_WARN("md: UpdateDeclination failed, could not load geoid data file!");
		goto exit;
	}

	CoordGeodetic.phi = latitude;
	CoordGeodetic.lambda = longitude;
	Geoid.UseGeoid = 1;
	CoordGeodetic.HeightAboveEllipsoid = elevation / 1000;
	WMM_ConvertGeoidToEllipsoidHeight(&CoordGeodetic, &Geoid);
	UserDate.Month =  ltime->tm_mon + 1;
	UserDate.Day = ltime->tm_mday;
	UserDate.Year = ltime->tm_year + 1900;

	if(!WMM_DateToYear(&UserDate, Error_Message))
	{
		PLAYER_WARN1("md: UpdateDeclination failed with message = %s", Error_Message);
		goto exit;
	}

	WMM_GeodeticToSpherical(Ellip, CoordGeodetic, &CoordSpherical);    /*Convert from geodeitic to Spherical Equations: 17-18, WMM Technical report*/
	WMM_TimelyModifyMagneticModel(UserDate, MagneticModel, TimedMagneticModel); /* Time adjust the coefficients, Equation 19, WMM Technical report */
	WMM_Geomag(Ellip, CoordSpherical, CoordGeodetic, TimedMagneticModel, &GeoMagneticElements);   /* Computes the geoMagnetic field elements and their time change*/
	WMM_CalculateGridVariation(CoordGeodetic,&GeoMagneticElements);

	declination = GeoMagneticElements.Decl;
	ret = true;

exit:
	if (MagneticModel)
	{
		WMM_FreeMagneticModelMemory(MagneticModel);
		MagneticModel = 0;
	}

	if (TimedMagneticModel)
	{
		WMM_FreeMagneticModelMemory(TimedMagneticModel);
		TimedMagneticModel = 0;
	}

	if (Geoid.GeoidHeightBuffer)
	{
		free(Geoid.GeoidHeightBuffer);
		Geoid.GeoidHeightBuffer = 0;
	}

	return ret;
}



Driver* driver_init(ConfigFile* cf, int section) {
	return new MagneticDeclinationDriver(cf, section);
}

void driver_register(DriverTable* table) {
	table->AddDriver("amosmd", driver_init);
}

extern "C" {
	int player_driver_init(DriverTable* table) {
		driver_register(table);
		return 0;
	}
}


#include "utm.h"
#include <assert.h>
#include <math.h>

// WGS84 Parameters
#define WGS84_A         6378137.0               // major axis
#define WGS84_B         6356752.31424518        // minor axis
#define WGS84_F         0.0033528107            // ellipsoid flattening
#define WGS84_E         0.0818191908            // first eccentricity
#define WGS84_EP        0.0820944379            // second eccentricity

// UTM Parameters
#define UTM_K0          0.9996                  // scale factor
#define UTM_FE          500000.0                // false easting
#define UTM_FN_N        0.0                     // false northing on north hemisphere
#define UTM_FN_S        10000000.0              // false northing on south hemisphere
#define UTM_E2          (WGS84_E*WGS84_E)       // e^2
#define UTM_E4          (UTM_E2*UTM_E2)         // e^4
#define UTM_E6          (UTM_E4*UTM_E2)         // e^6
#define UTM_EP2         (UTM_E2/(1-UTM_E2))     // e'^2

#define UTM_M0			((double)(1 - UTM_E2/4 - 3*UTM_E4/64 - 5*UTM_E6/256))
#define UTM_M1			((double)(-(3*UTM_E2/8 + 3*UTM_E4/32 + 45*UTM_E6/1024)))
#define UTM_M2			((double)(15*UTM_E4/256 + 45*UTM_E6/1024))
#define UTM_M3			((double)(-(35*UTM_E6/3072)))

void utm(double lat, double lon, double *x, double *y)
{
	assert(x && y);

	// compute the central meridian
	const int cm = (lon >= 0.0) ? ((int)lon - ((int)lon)%6 + 3) : ((int)lon - ((int)lon)%6 - 3);

	// convert degrees into radians
	const double rlat = lat * M_PI/180;
	const double rlon = lon * M_PI/180;
	const double rlon0 = cm * M_PI/180;

	// compute trigonometric functions
	const double slat = sin(rlat);
	const double clat = cos(rlat);
	const double tlat = tan(rlat);

	// decide the flase northing at origin
	const double fn = (lat > 0) ? UTM_FN_N : UTM_FN_S;

	const double T = tlat * tlat;
	const double C = UTM_EP2 * clat * clat;
	const double A = (rlon - rlon0) * clat;
	const double M = WGS84_A * (UTM_M0*rlat + UTM_M1*sin(2*rlat) + UTM_M2*sin(4*rlat) + UTM_M3*sin(6*rlat));
	const double V = WGS84_A / sqrt(1 - UTM_E2*slat*slat);

	// compute the easting-northing coordinates
	*x = UTM_FE + UTM_K0 * V * (A + (1-T+C)*pow(A,3)/6 + (5-18*T+T*T+72*C-58*UTM_EP2)*pow(A,5)/120);
	*y = fn + UTM_K0 * (M + V * tlat * (A*A/2 + (5-T+9*C+4*C*C)*pow(A,4)/24 + (61-58*T+T*T+600*C-330*UTM_EP2)*pow(A,6)/720));
}


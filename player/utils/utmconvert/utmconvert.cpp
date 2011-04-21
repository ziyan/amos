#include <iostream>
#include <cstdlib>
#include <cstdio>
#include "utm/utm.h"

using namespace std;

int main(int argc, char **argv)
{
	double lat, lon;
	double utm_e, utm_n;

	if (argc != 1 && argc != 3)
	{
		fprintf(stderr, "Usage: %s [<latitude> <longitude>]\n\n", argv[0]);
		return -1;
	}
	
	if (argc == 3)
	{
		utm(atof(argv[1]), atof(argv[2]), &utm_e, &utm_n);
		printf("%f %f\n", utm_e, utm_n);
		return 0;
	}
	
	while (cin)
	{
		cin >> lat >> lon;
		if (!cin) break;
		utm(lat, lon, &utm_e, &utm_n);
		printf("%f %f\n", utm_e, utm_n);
	}

	return 0;
}


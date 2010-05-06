// OGRMakeWktCoordinate bug
#include "ogr_geometry.h"
#include "ogr_p.h"
#include <cstdio>
#include <cmath>
using namespace std;

int main(int argc, char ** argv) {
	
	printf("A extreme but normal double value = 0xffffffffffefffff\n"); 
	long dd[2] = { 0xffffffff, 0xffefffff };
	double value = *((double*) dd);
	
	printf("  printed with %%g = %g\n", value);
	printf("  isnormal(value) returns %d\n", isnormal(value));
	printf("  printed with %%.15f, the format used by OGRMakeWktCoordinate, = %.15f\n", value);
		
	// now, the bug:
    char szCoordinate[80];   // as in OGRPoint::exportToWkt

    OGRMakeWktCoordinate(szCoordinate, value, 0.0, 0.0, 2);
	
	// we dont get this far -> segfault!
	
	return 0;
}


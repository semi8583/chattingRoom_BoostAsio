#include "logger.h"


ostreamFork::ostreamFork(ostream& os_one, ostream& os_two)
	: os1(os_one),
	os2(os_two)
{}
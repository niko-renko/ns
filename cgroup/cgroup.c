#include "../common.h"
#include "cgroup.h"

void configure_cgroup(void) {
	if (mount("cgroup", CGROUP_ROOT, "cgroup2", 0, NULL) < 0 && errno != EBUSY)
		die("cgroup");
}
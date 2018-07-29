//
// main.cpp
//
// Copyright (C) 2017-2018 Ken Hilton, all rights reserved.
//
// The contents of this source code is protected by trade secret law and may not be viewed,
// studied, compiled or otherwise utilized in any manner with out executing a binding non-
// disclosure agreement including written permission of permissabe from both Ken Hilton and
// Day Goodwin.  Furthermore, this source code contains both intellectual property and trade
// secrets that are the exclusive propery of Ken Hilton and Day Goodwin, respectively.
//

#include "fast_bitstring.h"
#include "test.h"


int
main(int argc, char *argv[]) {

	if (argc > 1) {
		exit(unit_test());
	}

	return 0;
}



#include <ctype.h>
#include <stdio.h>
#include <iostream>
#include <assert.h>

#include "sat_reader.hpp"
#include "util/sys/terminator.hpp"

bool SatReader::read(JobDescription& desc) {

	FILE* f;
	bool piped;
	if ((_filename.size() > 3 && _filename.substr(_filename.size()-3, 3) == ".xz")
		|| (_filename.size() > 5 && _filename.substr(_filename.size()-5, 5) == ".lzma")) {
		// Decompress, read output
		auto command = "xz -c -d " + _filename;
		f = popen(command.c_str(), "r");
		piped = true;
	} else {
		// Directly read file
    	f = fopen(_filename.c_str(), "r");
		piped = false;
	}
	if (f == NULL) {
		return false;
	}
	
	desc.beginInitialization();
	
	int sign = 1;
	bool comment = false;
	bool beganNum = false;
	int num = 0;
	int maxVar = 0;

	// Read every character of the formula (in a buffered manner)
	char buffer[4096] = {'\0'};
	while (!Terminator::isTerminating() && fgets(buffer, sizeof(buffer), f)) {
		size_t pos = 0;
		while (buffer[pos] != '\0') {

			int c = buffer[pos++];

			if (comment && c != '\n') continue;

			switch (c) {
			case '\n':
				comment = false;
				if (beganNum) {
					assert(num == 0);
					desc.addLiteral(0);
					beganNum = false;
				}
				break;
			case 'p':
			case 'c':
				comment = true;
				break;
			case ' ':
				if (beganNum) {
					maxVar = std::max(maxVar, num);
					desc.addLiteral(sign * num);
					num = 0;
					beganNum = false;
				}
				sign = 1;
				break;
			case '-':
				sign = -1;
				beganNum = true;
				break;
			default:
				num = num*10 + (c-'0');
				beganNum = true;
				break;
			}
		}
	}
	if (beganNum) { // final zero (without newline)
		desc.addLiteral(0);
	}

	desc.setNumVars(maxVar);
	desc.endInitialization();

	if (piped) pclose(f);
	else fclose(f);

	return true;
}

/*
 * configuration.cc
 *
 *  Created on: 2016年9月6日
 *      Author: xie
 */

#include "configuration.h"
#include "ts/ts.h"
#include <fstream>
#include <algorithm>
#include <vector>

void ltrim_if(std::string& s, int (*fp)(int)) {
	for (size_t i = 0; i < s.size();) {
		if (fp(s[i])) {
			s.erase(i, 1);
			continue;
		} else {
			break;
		}
	}
}

void rtrim_if(std::string& s, int (*fp)(int)) {
	for (size_t i = s.size() - 1; i >= 0; i--) {
		if (fp(s[i])) {
			s.erase(i, 1);
			continue;
		} else {
			break;
		}
	}
}

void trim_if(std::string& s, int (*fp)(int)) {
	ltrim_if(s, fp);
	rtrim_if(s, fp);
}

bool Configuration::Parse(const std::string fname) {
	std::ifstream f;
	std::string filename;
	size_t lineno = 0;
	if (0 == fname.size()) {
		TSDebug(PLUGIN_NAME,"no config filename provided");
		return false;
	}
	if (fname[0] != '/') {
		filename = TSConfigDirGet();
		filename += "/" + fname;
	} else {
		filename = fname;
	}
	f.open(filename.c_str(), std::ios::in);
	if (!f.is_open()) {
		TSDebug(PLUGIN_NAME," unable to open %s", filename.c_str());
		return false;
	}
	char temp[100];
	char *p, *key, *bp;
	uint64_t ssize;

	memset(temp, '\0', sizeof(temp));

	while (!f.eof()) {
		std::string line;
		getline(f, line);
		++lineno;

		trim_if(line, isspace);
		if (line.size() == 0)
			continue;

		bp = (char *) line.c_str();
		p = strstr(bp, "=");

		if (p) {
			memset(temp, '\0', sizeof(temp));
			strncpy(temp, bp, p - bp);
			key = (char *) malloc(sizeof(temp));
			memset(key, '\0', sizeof(temp));
			strcpy(key, temp);
			memset(temp, '\0', sizeof(temp));
			strcpy(temp, p + 1);
			ssize = atol(temp);
			limitconf.insert(std::pair<const char *, uint64_t>(key, ssize));
		}
	}

	return true;
}


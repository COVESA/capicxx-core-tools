/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <CommonAPI/tests/PingProxy.h>

#include <iostream>
#include <cstdlib>
#include <climits>
#include <getopt.h>

#include "Benchmark.h"


static bool verbose;
static bool async;
static int sendMethodType;
static unsigned int sendCount;
static unsigned int arraySize;

static struct option longOptions[] = {
		{ "help", no_argument, 0, 'h' },
		{ "verbose", no_argument, 0, 'v' },
		{ "async", no_argument, 0, 'A' },
		{ "count", required_argument, 0, 'c' },
		{ "empty", no_argument, &sendMethodType, CommonAPI::tests::Benchmark::SEND_TYPE_EMPTY },
		{ "copy", no_argument, &sendMethodType, CommonAPI::tests::Benchmark::SEND_TYPE_COPY },
		{ "copies", required_argument, &sendMethodType, CommonAPI::tests::Benchmark::SEND_TYPE_COPIES },
		{ 0, 0, 0, 0 }
};

static void showUsage(char *programPath) {
	std::cout << "Usage: " << programPath << " OPTIONS\n"
			<< " Generic options:"
			<< "  --verbose\n"
			<< "  --async               Perform async (otherwise sync) remote method calls.\n"
			<< "  --count <number>      Number of messages to send.\n"
			<< " Send mode options:\n"
			<< "  --empty               Send an empty message.\n"
			<< "  --copy                Send a single copy of the last test struct.\n"
			<< "  --copies <size>       Send a copy of the last arguments <size> array of test struct.\n"
			<< " TestStruct contents:\n"
            << "  int32_t int32Value   (4 bytes)\n"
            << "  double  doubleValue1 (8 bytes)\n"
            << "  double  doubleValue2 (8 bytes)\n"
            << "  string  stringValue  (20 chars)\n";
}

static bool parseCommandLineArguments(int argc, char **argv) {
	int ch;
	int optionIndex = 0;

	while ((ch = getopt_long(argc, argv, "Ahvc:", longOptions, &optionIndex)) != -1) {
		switch (ch) {
		case 0:
			// save the size parameter of the "copies" option
			if (longOptions[optionIndex].has_arg) {
				arraySize = strtoul(optarg, NULL, 10);
				if (sendCount == ULONG_MAX) {
					std::cerr << "Invalid size value: " << optarg << std::endl;
					return false;
				}
			}
			break;

		case 'c':
			sendCount = strtoul(optarg, NULL, 10);
			if (sendCount == ULONG_MAX) {
				std::cerr << "Invalid count value: " << optarg << std::endl;
				return false;
			}
			break;

		case 'A':
			async = true;
			break;

		case 'v':
			verbose = true;
			break;

		case 'h':
			showUsage(argv[0]);
			return false;

		case '?':
			return false;
		}
	}

	if (!sendCount) {
		std::cerr << "Missing send count argument! See --help\n";
		return false;
	}

	return true;
}

int main(int argc, char** argv) {
	if (!parseCommandLineArguments(argc, argv))
		return -1;

	const CommonAPI::tests::Benchmark::SendType benchmarkSendType = (CommonAPI::tests::Benchmark::SendType) sendMethodType;
	CommonAPI::tests::Benchmark benchmark(benchmarkSendType, sendCount, arraySize, verbose, async);

	if (!benchmark.run())
		return -1;

    return 0;
}

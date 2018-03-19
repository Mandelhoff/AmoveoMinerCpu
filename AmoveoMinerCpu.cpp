#define VERSION_STRING "2.0.0.0"
#define TOOL_NAME "AmoveoMinerCpu"

#include <stdio.h>
#include <iostream>
#include <chrono>
#include <cmath>
#include <thread>
#include <iomanip>
#include <string>
#include <cassert>

#include <iostream>
#include <chrono>
#include <cmath>
#include <thread>
#include <iomanip>
#include <string>
#include <cassert>

#include <future>
#include <numeric>
#include <chrono>

#include <cpprest/http_client.h>
#include <cpprest/filestream.h>
#include <cpprest/http_listener.h>              // HTTP server
#include <cpprest/json.h>                       // JSON library
#include <cpprest/uri.h>                        // URI library
#include <cpprest/asyncrt_utils.h>

#include <openssl/sha.h>
#include "base64.h"
#include "poolApi.h"

typedef unsigned char BYTE;             // 8-bit byte

using namespace std;
using namespace std::chrono;

using namespace utility;									// Common utilities like string conversions
using namespace web;										// Common features like URIs.
using namespace web::http;									// Common HTTP functionality
using namespace web::http::client;							// HTTP client features
using namespace concurrency::streams;						// Asynchronous streams
using namespace web::http::experimental::listener;          // HTTP server
using namespace web::json;                                  // JSON library


//#define POOL_URL "http://localhost:32371/work"	// local pool
//#define POOL_URL "http://amoveopool.com/work"
#define POOL_URL "http://amoveopool2.com/work"

#define MINER_THREADS 4

#define MINER_ADDRESS "BPA3r0XDT1V8W4sB14YKyuu/PgC6ujjYooVVzq1q1s5b6CAKeu9oLfmxlplcPd+34kfZ1qx+Dwe3EeoPu0SpzcI="


string gMinerPublicKeyBase64(MINER_ADDRESS);
int gMinerThreads = MINER_THREADS;
string gPoolUrl(POOL_URL);
string_t gPoolUrlW;

PoolApi gPoolApi;
WorkData gWorkData;
std::mutex mutexWorkData;
Metrics gMetrics;


// Prints a 32 bytes sha256 to the hexadecimal form filled with zeroes
void print_hash(const unsigned char* sha256) {
	for (size_t i = 0; i < 32; ++i) {
		std::cout << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(sha256[i]);
	}
	std::cout << std::dec << std::endl;
}

unsigned int hash2integer(unsigned char * h) {
	int x = 0;
	int z = 0;
	for (int i = 0; i < 31; i++)
	{
		if (h[i] == 0)
		{
			x += 8;
			continue;
		}
		else if (h[i] < 2)
		{
			x += 7;
			z = h[i + 1];
		}
		else if (h[i] < 4)
		{
			x += 6;
			z = (h[i + 1] / 2) + ((h[i] % 2) * 128);
		}
		else if (h[i] < 8)
		{
			x += 5;
			z = (h[i + 1] / 4) + ((h[i] % 4) * 64);
		}
		else if (h[i] < 16)
		{
			x += 4;
			z = (h[i + 1] / 8) + ((h[i] % 8) * 32);
		}
		else if (h[i] < 32)
		{
			x += 3;
			z = (h[i + 1] / 16) + ((h[i] % 16) * 16);
		}
		else if (h[i] < 64)
		{
			x += 2;
			z = (h[i + 1] / 32) + ((h[i] % 32) * 8);
		}
		else if (h[i] < 128)
		{
			x += 1;
			z = (h[i + 1] / 64) + ((h[i] % 64) * 4);
		}
		else
		{
			z = (h[i + 1] / 128) + ((h[i] % 128) * 2);
		}
		break;
	}

	return ((256 * x) + z);
}

static void submitwork_thread(unsigned char * nonceSolution)
{
	gPoolApi.SubmitWork(gPoolUrlW, base64_encode(nonceSolution, 23), gMinerPublicKeyBase64);
}

uint64_t gNonceEnding = 0;
std::mutex mutexNonceEnding;

#define SHA_LOOP_COUNT	65536

static bool miner_thread()
{
	int shareDifficulty = 0;
	SHA256_CTX ctx;
	unsigned char hashResult[32];

	while (true)
	{
		mutexNonceEnding.lock();
		uint64_t myNonceEnding = gNonceEnding++;
		mutexNonceEnding.unlock();

		mutexWorkData.lock();
		gWorkData.getCtx(&ctx);
		shareDifficulty = gWorkData.shareDifficulty;
		mutexWorkData.unlock();

		SHA256_Update(&ctx, &myNonceEnding, 6);

		SHA256_CTX ctxTmp;
		for (int idx = 0; idx < SHA_LOOP_COUNT; idx++)
		{
			memcpy(&ctxTmp, &ctx, 0x70);
			SHA256_Update(&ctxTmp, &idx, 2);
			SHA256_Final(hashResult, &ctxTmp);

			if (hash2integer(hashResult) >= gWorkData.shareDifficulty)
			{
				unsigned char solutionNonce[23];
				memcpy(solutionNonce, &gWorkData.nonce[0], 15);
				memcpy(solutionNonce + 15, &myNonceEnding, 6);
				memcpy(solutionNonce + 21, &idx, 2);
				std::async(std::launch::async, submitwork_thread, std::ref(solutionNonce));
			}
		}
		gMetrics.addHashesTried(SHA_LOOP_COUNT);
	}

	return true;
}

static bool getwork_thread(std::seed_seq &seed)
{
	std::independent_bits_engine<std::default_random_engine, 32, uint32_t> randomBytesEngine(seed);

	SHA256_CTX ctxBuf;

	while (true)
	{
		WorkData workDataNew;
		gPoolApi.GetWork(gPoolUrlW, &workDataNew, gMinerPublicKeyBase64);

		// Check if new work unit is actually different than what we currently have
		if (memcmp(&gWorkData.bhash[0], &workDataNew.bhash[0], 32) != 0) {
			mutexWorkData.lock();
			std::generate(begin(gWorkData.nonce), end(gWorkData.nonce), std::ref(randomBytesEngine));
			gWorkData.bhash = workDataNew.bhash;
			gWorkData.blockDifficulty = workDataNew.blockDifficulty;
			gWorkData.shareDifficulty = workDataNew.shareDifficulty;

			SHA256_Init(&ctxBuf);
			SHA256_Update(&ctxBuf, &gWorkData.bhash[0], 32);
			SHA256_Update(&ctxBuf, &gWorkData.nonce[0], 15);

			gWorkData.setCtx(&ctxBuf);

			mutexWorkData.unlock();

			std::cout << "New Work: BDiff:" << gWorkData.blockDifficulty << " SDiff:" << gWorkData.shareDifficulty << endl;
		}
		else {
			// Even if new work is not available, shareDiff will likely change. Need to adjust, else will get a "low diff share" error.
			gWorkData.shareDifficulty = workDataNew.shareDifficulty;
		}

		_sleep(4000);
	}

	return true;
}

std::string gSeedStr("ImAraNdOmStrInG");

int main(int argc, char* argv[])
{
	cout << TOOL_NAME << " v" << VERSION_STRING << endl;
	if (argc <= 1) {
		cout << "Example Template: " << endl;
		cout << argv[0] << " " << "<Base64AmoveoAddress>" << " " << "<Threads>" << " " << "<RandomSeed>" << " " << "<PoolUrl>" << endl;

		cout << endl;
		cout << "Example Usage: " << endl;
		cout << argv[0] << " " << MINER_ADDRESS << " " << MINER_THREADS << " " << gSeedStr << " " << POOL_URL << endl;

		cout << endl;
		cout << endl;
		cout << "Threads is optional. Default Threads is 4." << endl;
		cout << "RandomSeed is optional. Set this if you get many Duplicate Nonce errors" << endl;
		cout << "PoolUrl is optional. Default PoolUrl is http://amoveopool2.com/work" << endl;
		return -1;
	}
	if (argc >= 2) {
		gMinerPublicKeyBase64 = argv[1];
	}
	if (argc >= 3) {
		gMinerThreads = atoi(argv[2]);
	}
	if (argc >= 4) {
		gSeedStr = argv[3];
	}
	if (argc >= 5) {
		gPoolUrl = argv[4];
	}

	cout << "Wallet Address: " << gMinerPublicKeyBase64 << endl;
	cout << "CPU Threads: " << gMinerThreads << endl;
	cout << "Random Seed: " << gSeedStr << endl;
	cout << "Pool Url: " << gPoolUrl << endl;

	std::seed_seq seed(gSeedStr.begin(), gSeedStr.end());
	future<bool> getWorkThread = std::async(std::launch::async, getwork_thread, std::ref(seed));

	gPoolUrlW.resize(gPoolUrl.length(), L' ');
	std::copy(gPoolUrl.begin(), gPoolUrl.end(), gPoolUrlW.begin());

	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

	while (!gWorkData.HasNewWork())
	{
		mySleep(100);
	}

	std::chrono::steady_clock::time_point workDataBegin = std::chrono::steady_clock::now();

	vector<future<bool>> minerFutures;
	for (int idx = 0; idx < gMinerThreads; idx++) {
		minerFutures.push_back(std::async(std::launch::async, miner_thread));
	}

	int64_t secondDuration = 0;
	while (true)
	{
		mySleep(4000);

		std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
		secondDuration = std::chrono::duration_cast<std::chrono::seconds>(now - begin).count();
		cout << "H/S: " << gMetrics.getHashesTried() / secondDuration << endl;
	}

	return 0;
}


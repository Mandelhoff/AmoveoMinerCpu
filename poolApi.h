#pragma once


#include <string>
#include <vector>
#include <openssl\sha.h>

using namespace std;
using namespace utility;                    // Common utilities like string conversions

void mySleep(unsigned milliseconds);

class Metrics
{
	uint64_t hashesTried;
	std::mutex mutex;
public:
	Metrics() {
		hashesTried = 0;
	}
	uint64_t getHashesTried() { return hashesTried; }
	void addHashesTried(uint64_t hashCount)
	{
		mutex.lock();
		hashesTried += hashCount;
		mutex.unlock();
	}
};

class WorkData
{
private:
	SHA256_CTX ctx;
	bool newWork = false;

public:
	vector<unsigned char> bhash;
	vector<unsigned char> nonce;
	int blockDifficulty;
	int shareDifficulty;

	std::mutex mutexNewWork;
	//std::mutex mutexCtx;

public:
	WorkData() {
		bhash = vector<unsigned char>(32);
		nonce = vector<unsigned char>(15);
	}

	bool HasNewWork() { return newWork; }
	void clearNewWork()
	{
		mutexNewWork.lock();
		newWork = false;
		mutexNewWork.unlock();
	}

	void getCtx(SHA256_CTX * pCtx) {
		//mutexCtx.lock();
		memcpy(pCtx, &ctx, 0x70);
		//mutexCtx.unlock();
	}
	void setCtx(SHA256_CTX * pCtx)
	{
		//mutexCtx.lock();
		memcpy(&ctx, pCtx, 0x70);
		//mutexCtx.unlock();
		mutexNewWork.lock();
		newWork = true;
		mutexNewWork.unlock();
	}
};

class PoolApi
{
public:
	void GetWork(string_t poolUrl, WorkData * pMinerThreadData, string minerPublicKeyBase64);
	void SubmitWork(string_t poolUrl, string nonceBase64, string minerPublicKeyBase64);


};
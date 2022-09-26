#include "GameTimer.h"

GameTimer::GameTimer()
	: mSecondsPerCount(0.0),
	mDeltaTime(-1.0),
	mBaseTime(0),
	mPausedTimeInterval(0),
	mLastPasedTime(0),
	mPrevTime(0),
	mCurrTime(0),
	mStopped(false) {
	INT64 countsPerSec = 0;
	QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);
	mSecondsPerCount = 1.0 / (double)countsPerSec;
}

float GameTimer::TotalTime() const {
	if (mStopped) {
		return (mLastPasedTime - mBaseTime - mPausedTimeInterval) * mSecondsPerCount;
	}
	
	return (mCurrTime - mBaseTime - mPausedTimeInterval) * mSecondsPerCount;
}

float GameTimer::DeltaTime() const {
	return mDeltaTime;
}

void GameTimer::Reset() {
	INT64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

	mBaseTime = currTime;
	mPrevTime = currTime;
	mPausedTimeInterval = 0;
	mStopped = false;
}

void GameTimer::Start() {
	if (mStopped) {
		INT64 currTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

		mPausedTimeInterval += currTime - mLastPasedTime;
		mPrevTime = currTime;
		mCurrTime = currTime;
		mStopped = false;
	}
}

void GameTimer::Pause() {
	if (!mStopped) {
		INT64 currTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

		mLastPasedTime = currTime;
		mStopped = true;
	}
}

void GameTimer::Tick() {
	if (mStopped) {
		mDeltaTime = 0.0;
		return;
	}

	INT64 currTime = 0;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
	mCurrTime = currTime;

	mDeltaTime = (mCurrTime - mPrevTime) * mSecondsPerCount;
	mPrevTime = mDeltaTime;

	if (mDeltaTime < 0.0) {
		mDeltaTime = 0.0;
	}
}

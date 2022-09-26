#pragma once
#include <windows.h>

class GameTimer {
public:
	GameTimer();

	float TotalTime() const;
	float DeltaTime() const;

	void Reset();
	void Start();
	void Pause();
	void Tick();

private:
	double mSecondsPerCount;
	double mDeltaTime;			// 两帧之间的时间差

	INT64 mBaseTime;			// 基准时间点
	INT64 mPausedTimeInterval;  // 暂停的时间总和
	INT64 mLastPasedTime;		// 上一次暂停的时间点
	INT64 mPrevTime;			// 上一次进行绘制的时间点
	INT64 mCurrTime;			// 当前时间

	bool mStopped;
};
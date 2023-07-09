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
	double mDeltaTime;			// ��֮֡���ʱ���

	INT64 mBaseTime;			// ��׼ʱ���
	INT64 mPausedTimeInterval;  // ��ͣ��ʱ���ܺ�
	INT64 mLastPasedTime;		// ��һ����ͣ��ʱ���
	INT64 mPrevTime;			// ��һ�ν��л��Ƶ�ʱ���
	INT64 mCurrTime;			// ��ǰʱ��

	bool mStopped;
};
//***************************************************************************************
// GameTimer.cpp by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#include <windows.h>
#include "GameTimer.h"

GameTimer::GameTimer()
: m_secondsPerCount(0.0), m_deltaTime(-1.0), m_baseTime(0),
m_pausedTime(0), m_previousTime(0), m_currentTime(0), m_isStopped(false)
{
	__int64 countsPerSec;
	QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);
	m_secondsPerCount = 1.0 / (double)countsPerSec;
}

// Returns the total time elapsed since Reset() was called, NOT counting any
// time when the clock is stopped.
float GameTimer::TotalTime()const
{
	// If we are stopped, do not count the time that has passed since we stopped.
	// Moreover, if we previously already had a pause, the distance 
	// mStopTime - m_baseTime includes paused time, which we do not want to count.
	// To correct this, we can subtract the paused time from mStopTime:  
	//
	//                     |<--paused time-->|
	// ----*---------------*-----------------*------------*------------*------> time
	//  m_baseTime       mStopTime        startTime     mStopTime    mCurrTime

	if(m_isStopped)
	{
		return (float)(((m_stopTime - m_pausedTime)- m_baseTime)* m_secondsPerCount);
	}

	// The distance mCurrTime - m_baseTime includes paused time,
	// which we do not want to count.  To correct this, we can subtract 
	// the paused time from mCurrTime:  
	//
	//  (mCurrTime - m_pausedTime) - m_baseTime 
	//
	//                     |<--paused time-->|
	// ----*---------------*-----------------*------------*------> time
	//  m_baseTime       mStopTime        startTime     mCurrTime
	
	else
	{
		return (float)(((m_currentTime - m_pausedTime)- m_baseTime)* m_secondsPerCount);
	}
}

float GameTimer::DeltaTime()const
{
	return (float)m_deltaTime;
}

float GameTimer::SecondsPerCount()const
{
	return (float)m_secondsPerCount;
}

void GameTimer::Reset()
{
	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

	m_baseTime = currTime;
	m_previousTime = currTime;
	m_stopTime = 0;
	m_isStopped = false;
}

void GameTimer::Start()
{
	__int64 startTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&startTime);


	// Accumulate the time elapsed between stop and start pairs.
	//
	//                     |<-------d------->|
	// ----*---------------*-----------------*------------> time
	//  m_baseTime       m_stopTime        startTime     

	if(m_isStopped)
	{
		m_pausedTime += (startTime - m_stopTime);	//累计暂停的时间

		m_previousTime = startTime;
		m_stopTime = 0;
		m_isStopped = false;
	}
}

void GameTimer::Stop()
{
	if(!m_isStopped)
	{
		__int64 currTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

		m_stopTime = currTime;//记录暂停开始的时间
		m_isStopped = true;
	}
}

//每帧调用，计算上一帧到当前帧所花费的时间
void GameTimer::Tick()
{
	if (m_isStopped)
	{
		m_deltaTime = 0.0;
		return;
	}

	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
	m_currentTime = currTime;

	// Time difference between this frame and the previous.
	m_deltaTime = (m_currentTime - m_previousTime) * m_secondsPerCount;

	// Prepare for next frame.
	m_previousTime = m_currentTime;

	// Force nonnegative.  The DXSDK's CDXUTTimer mentions that if the 
	// processor goes into a power save mode or we get shuffled to another
	// processor, then m_deltaTime can be negative.
	if (m_deltaTime < 0.0)
		m_deltaTime = 0.0;
}
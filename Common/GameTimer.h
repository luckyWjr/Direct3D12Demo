#ifndef GAMETIMER_H
#define GAMETIMER_H

class GameTimer
{
public:
	GameTimer();

	float TotalTime()const;
	float DeltaTime()const;			// return m_deltaTime
	float SecondsPerCount()const;	// return m_secondsPerCount

	void Reset();	// 初始化、重置
	void Start();	// Call when unpaused.
	void Stop();	// Call when paused.
	void Tick();	// Call every frame.

private:
	double m_secondsPerCount;	// 秒/count
	double m_deltaTime;			// 上一帧到当前帧花费的时间，单位second

	// 下列变量的单位为：count
	__int64 m_baseTime;			// 计时器初始化时的时间
	__int64 m_pausedTime;		// 累计暂停的时间
	__int64 m_stopTime;			// 计时器暂停开始时间
	__int64 m_previousTime;		// 上一帧的开始时间
	__int64 m_currentTime;		// 当前帧的开始时间

	bool m_isStopped;			// 是否暂停
};

#endif
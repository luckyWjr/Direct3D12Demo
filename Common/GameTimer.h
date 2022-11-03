#ifndef GAMETIMER_H
#define GAMETIMER_H

class GameTimer
{
public:
	GameTimer();

	float TotalTime()const;
	float DeltaTime()const;			// return m_deltaTime
	float SecondsPerCount()const;	// return m_secondsPerCount

	void Reset();	// ��ʼ��������
	void Start();	// Call when unpaused.
	void Stop();	// Call when paused.
	void Tick();	// Call every frame.

private:
	double m_secondsPerCount;	// ��/count
	double m_deltaTime;			// ��һ֡����ǰ֡���ѵ�ʱ�䣬��λsecond

	// ���б����ĵ�λΪ��count
	__int64 m_baseTime;			// ��ʱ����ʼ��ʱ��ʱ��
	__int64 m_pausedTime;		// �ۼ���ͣ��ʱ��
	__int64 m_stopTime;			// ��ʱ����ͣ��ʼʱ��
	__int64 m_previousTime;		// ��һ֡�Ŀ�ʼʱ��
	__int64 m_currentTime;		// ��ǰ֡�Ŀ�ʼʱ��

	bool m_isStopped;			// �Ƿ���ͣ
};

#endif
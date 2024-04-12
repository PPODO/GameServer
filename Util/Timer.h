#pragma once
#include <iostream>
#include <functional>
#include <chrono>
#include <thread>
#include <mutex>
#include <array>
#include <list>
#include <Functions/CriticalSection/CriticalSection.hpp>
#include <Functions/Log/Log.hpp>

const size_t MAX_WHEEL_SIZE = 7;

class TimerDelegate {
public:
	const size_t m_iTimeInterval;

private:
	size_t m_iRotationCount;
	std::function<void()> m_callbackFunction;

public:
	TimerDelegate(const size_t iTimeInterval) : m_iTimeInterval(iTimeInterval), m_iRotationCount(0) {};
	TimerDelegate(const size_t iTimeInterval, const std::function<void()>& callbackFunction) : m_iTimeInterval(iTimeInterval), m_iRotationCount(0), m_callbackFunction(callbackFunction) {};

public:
	template<typename F, typename ...Argc>
	void BindDelegate(F&& Function, Argc&&... argc) {
		m_callbackFunction = std::bind(std::forward<F>(Function), std::forward<Argc>(argc)...);
	}

public:
	inline void SetRotationCount(const size_t& iRotationCount) { m_iRotationCount = iRotationCount; }
	inline size_t GetRotationCount() const { return m_iRotationCount; }
	inline bool CallbackFunctionCalling() { if (m_callbackFunction) { m_callbackFunction(); return true; } return false; };

};

class TimerClass {
private:
	volatile size_t m_iCurrentIndex;
	volatile size_t m_iCurrentRotationCount;

	const size_t m_iSlotInterval;

private:
	bool m_bIsStop;

	std::thread m_timerThread;
	SERVER::FUNCTIONS::CRITICALSECTION::CriticalSection m_criticalSection;

	std::array<std::list<TimerDelegate>, ::MAX_WHEEL_SIZE> m_TimingWheel;
	std::chrono::system_clock::time_point m_startTime, m_lastIntervalTime;

private:
	TimerClass(TimerClass& tc) : TimerClass(tc.m_iSlotInterval) {}

public:
	TimerClass(const size_t& SlotInterval) : m_iSlotInterval(SlotInterval), m_iCurrentIndex(0), m_iCurrentRotationCount(0), m_bIsStop(false), m_startTime(), m_lastIntervalTime() {
		m_startTime = std::chrono::system_clock::now();

		m_timerThread = std::thread([this]() {
			while (!m_bIsStop) {
				if (std::chrono::system_clock::now() - m_lastIntervalTime > std::chrono::duration<size_t>(m_iSlotInterval / 1000)) {
					for (auto Iterator = m_TimingWheel[m_iCurrentIndex].begin(); Iterator != m_TimingWheel[m_iCurrentIndex].end();) {
						if (m_iCurrentRotationCount == Iterator->GetRotationCount()) {
							if (!Iterator->CallbackFunctionCalling())
								SERVER::FUNCTIONS::LOG::Log::WriteLog(TEXT("Failed To Calling Timer Callback Function!"));
							
							SERVER::FUNCTIONS::CRITICALSECTION::CriticalSectionGuard lck(m_criticalSection);
							Iterator = m_TimingWheel[m_iCurrentIndex].erase(Iterator);
						}
						else {
							Iterator++;
						}
					}

					if (m_iCurrentIndex + 1 >= ::MAX_WHEEL_SIZE) {
						m_iCurrentIndex = 0;
						m_iCurrentRotationCount++;
					}
					else
						m_iCurrentIndex++;

					m_lastIntervalTime = std::chrono::system_clock::now();
				}
			}
			});
	};

	~TimerClass() {
		m_bIsStop = true;
		m_timerThread.join();
	}

public:
	void AddNewTimer(TimerDelegate& ti) {
		size_t NewIndex = (m_iCurrentIndex + (ti.m_iTimeInterval / m_iSlotInterval));
		ti.SetRotationCount(NewIndex >= ::MAX_WHEEL_SIZE ? m_iCurrentRotationCount + 1 : m_iCurrentRotationCount);

		SERVER::FUNCTIONS::CRITICALSECTION::CriticalSectionGuard lck(m_criticalSection);
		m_TimingWheel[NewIndex % ::MAX_WHEEL_SIZE].push_back(ti);
	}

};
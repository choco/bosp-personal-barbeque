/*
 * time_monitor.cpp
 *
 */

#include "bbque/rtlib/monitors/time_monitor.h"
#include <ratio>

uint16_t TimeMonitor::newGoal(uint32_t goal)
{
	TimeWindow::Target target(DataFunction::Average,
				  ComparisonFunction::LessOrEqual,
				  goal);
	std::vector<TimeWindow::Target> targets;
	targets.push_back(target);

	return TimeMonitor::newGoal(targets, defaultWindowSize);
}

uint16_t TimeMonitor::newGoal(uint32_t goal, uint16_t windowSize)
{
	TimeWindow::Target target(DataFunction::Average,
				  ComparisonFunction::LessOrEqual,
				  goal);
	std::vector<TimeWindow::Target> targets;
	targets.push_back(target);

	return TimeMonitor::newGoal(targets, windowSize);
}

uint16_t TimeMonitor::newGoal(DataFunction fType,
			      ComparisonFunction cType,
			      uint32_t goal)
{
	TimeWindow::Target target(fType, cType, goal);
	std::vector<TimeWindow::Target> targets;
	targets.push_back(target);

	return TimeMonitor::newGoal(targets, defaultWindowSize);
}

uint16_t TimeMonitor::newGoal(DataFunction fType,
			      ComparisonFunction cType,
			      uint32_t goal,
			      uint16_t windowSize)
{
	TimeWindow::Target target(fType, cType, goal);
	std::vector<TimeWindow::Target> targets;
	targets.push_back(target);

	return TimeMonitor::newGoal(targets, windowSize);
}

uint16_t TimeMonitor::newGoal(std::vector<TimeWindow::Target> targets)
{
	return TimeMonitor::newGoal(targets, defaultWindowSize);
}

uint16_t TimeMonitor::newGoal(std::vector<TimeWindow::Target> targets,
			      uint16_t windowSize)
{
	TimeWindow * tWindow = new TimeWindow(targets, windowSize);

	tWindow->started = false;

	uint16_t id = getUniqueId();
	goalList[id] = tWindow;

	return id;
}

void TimeMonitor::resetGoal(uint16_t id)
{
	Monitor<uint32_t>::resetGoal(id);
	dynamic_cast<TimeWindow*> (goalList[id])->started = false;
}

void TimeMonitor::start(uint16_t id)
{
	if (goalList.find(id) == goalList.end())
		return;

	dynamic_cast<TimeWindow*> (goalList[id])->started = true;
	dynamic_cast<TimeWindow*> (goalList[id])->tStart =
					std::chrono::monotonic_clock::now();
}

void TimeMonitor::stop(uint16_t id)
{
	if (goalList.find(id) == goalList.end())
		return;

	if (dynamic_cast<TimeWindow*> (goalList[id])->started) {
		dynamic_cast<TimeWindow*> (goalList[id])->tStop =
			std::chrono::monotonic_clock::now();
		uint32_t elapsedTime =
			std::chrono::duration_cast<std::chrono::milliseconds > (
			dynamic_cast<TimeWindow*> (goalList[id])->tStop -
			dynamic_cast<TimeWindow*> (goalList[id])->tStart
			).count();
		goalList[id]->addElement(elapsedTime);
		dynamic_cast<TimeWindow*> (goalList[id])->started = false;
	}

}

void TimeMonitor::start()
{
	std::lock_guard<std::mutex> lg(timerMutex);
	started = true;
	tStart = std::chrono::monotonic_clock::now();
}

void TimeMonitor::stop()
{
	std::lock_guard<std::mutex> lg(timerMutex);
	if (started) {
		tStop = std::chrono::monotonic_clock::now();
		started = false;
	}
}

double TimeMonitor::getElapsedTime()
{
	if (started)
		stop();
	return (std::chrono::duration_cast<std::chrono::duration<double> >
				(tStop - tStart).count());
}

double TimeMonitor::getElapsedTimeMs()
{
	if (started)
		stop();
	return (std::chrono::duration_cast<
				std::chrono::duration<double, std::milli> >
				(tStop - tStart).count());
}

double TimeMonitor::getElapsedTimeUs()
{
	if (started)
		stop();
	return (std::chrono::duration_cast<
				std::chrono::duration<double, std::micro> >
				(tStop - tStart).count());
}
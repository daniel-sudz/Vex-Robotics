#pragma once
#include "pros/adi.hpp"

class LineTracker
{
public:
    LineTracker(unsigned int port);
    void Reset();
    void Update();
    unsigned int GetWhiteLineDistance(bool pop);
    bool HasWhiteLine(int distanceToTravel);
    int MinValue() { return m_minvalue; }

protected:
    enum class Status
    {
        None,
        HitWhite,
        HitBlack,
    };
    void Push(bool white);
    void Shift(unsigned int elements);
    int IndexForWhiteLine();
    void ResetCore();

protected:
    constexpr static int WhiteLevel = 1200;
    constexpr static int BlackLevel = 2000;

    pros::ADIAnalogIn m_sensor;
    unsigned int m_times[8];
    unsigned int m_timesIndex = 0;

    int m_minvalue = 4000;

    Status m_status = Status::None;
};

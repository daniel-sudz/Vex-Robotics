#pragma once
#include "main.h"
#include "cycle.h"
#include "logger.h"

#include <math.h>
#include <cstdio>

void SetShooterAngle(bool hightFlag, int distance);
inline void IntakeUp() { GetMain().intake.SetIntakeDirection(Intake::Direction::Up); }
inline void IntakeDown() { GetMain().intake.SetIntakeDirection(Intake::Direction::Down); }
inline void IntakeStop() { GetMain().intake.SetIntakeDirection(Intake::Direction::None); }

struct Action
{
    Main &m_main = GetMain();

    Action() { m_timeStart = m_main.GetTime(); }
    unsigned int GetElapsedTime() const { return m_main.GetTime() - m_timeStart; }
    virtual bool ShouldStop() = 0; //{ return true; }
    virtual void Stop() {}
    virtual const char* Name() { return "unknown"; }
private:
    unsigned int m_timeStart;
};

struct EndOfAction : public Action
{
    bool ShouldStop() override { return false; }
};

struct WaitAction : public Action
{
    unsigned int m_wait;
    WaitAction(unsigned int wait) : m_wait(wait) {}
    bool ShouldStop() override { return GetElapsedTime() >= m_wait; }
    const char* Name() override { return "Wait"; } 
};


struct WaitTillStopsAction : public Action
{
    bool ShouldStop() override
    {
        auto left = abs(GetLeftVelocity());
        auto right = abs(GetRightVelocity());
        return left <= 5 && right <= 5;
    }
    const char* Name() override { return "WaitTillStopsAction"; } 
};


struct WaitShooterAngleToStopAction : public Action
{
    bool ShouldStop() override
    {
         return !m_main.shooter.IsMovingAngle();
    }
    const char* Name() override { return "WaitShooterAngleToStopAction"; }
private:
};

struct WaitForBallAction : public Action
{
    bool ShouldStop() override
    {
        return (m_main.shooter.BallStatus() == BallPresence::HasBall);
    }

    const char* Name() override { return "WaitForBallAction"; }
};

struct ShootBallAction : public Action
{
    ShootBallAction()
    {
        Assert(!m_main.shooter.IsShooting());
        Assert(!m_main.shooter.IsMovingAngle());
        if (m_main.shooter.BallStatus() !=  BallPresence::NoBall)
        {
            m_main.shooter.OverrideSetShooterMode(true /*shooting*/);
            Assert(m_main.shooter.IsShooting());
        }
    }
    bool ShouldStop() override { return !m_main.shooter.IsShooting(); }
    void Stop() override { m_main.shooter.StopShooting(); } // in case we timed-out
    const char* Name() override { return "ShootBallAction"; }
};

struct ShootWithVisionAction : public Action
{
    ShootWithVisionAction(bool visionMove, bool visionAngle)
    {
        GetMain().vision.ShootingInAutonomous(visionMove, visionAngle);
        (void) GetMain().vision.GetAndResetFoundCount();
    }
    bool ShouldStop()
    {
        if ((GetElapsedTime() % 100) == 0)
        {
            auto ratio = GetMain().vision.GetAndResetFoundCount();
            ReportStatus("Vision aiming: %d / 10\n", ratio);
            if (ratio < 3)
                return true;
        }

        m_shot = !GetMain().vision.IsShooting();
        return m_shot;
    }
    void Stop() override
    {
        GetMain().vision.ShootingInAutonomous(false, false);
    }

    bool Shot() { return m_shot; }
    const char* Name() override { return "ShootWithVisionAction"; } 

private:
    bool m_shot = false;
};

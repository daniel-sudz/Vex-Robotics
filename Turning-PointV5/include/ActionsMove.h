#pragma once
#include <math.h>
#include <limits.h>
#include "actions.h"
#include "position.h"

extern const  bool g_leverageLineTrackers;

struct MoveAction : public Action
{
    const char* Name() override { return "MoveAction"; }

    MoveAction(int distance, int power = 85)
        : m_forward(distance >= 0)
    {
        Assert(distance != 0);
        if (!m_forward)
        {
            distance = -distance;
            power = -power;
        }
        m_distanceToMove = distance;
        m_origDistanceToMove = distance;
        
        m_main.drive.ResetTrackingState();
        Assert(m_main.drive.m_distance == 0);
        m_main.drive.OverrideInputs(power, 0/*turn*/);
    }

    bool ShouldStop() override
    {
        unsigned int distance = m_main.drive.m_distance;

        if (m_stopOnCollision)
        {
            PositionInfo info = GetTracker().LatestPosition(false /*clicks*/);
            int left = int(info.leftSpeed*1000);
            int right = int(info.rightSpeed*1000);

            if (abs(left) <= 10 || abs(right) <= 10)
            {
                ReportStatus("   Collision detected! distance %d / %d, speeds: %d, %d\n",
                        distance, m_origDistanceToMove, left, right);
                return true;
            }
        }

        if (m_main.drive.m_distance < m_distanceToMove)
            return false;

        if (m_stopOnCollision)
            ReportStatus("Move did not hit wall: dist = %d\n", m_origDistanceToMove);

        return true;
    }

    void Stop() override
    {
        m_main.drive.OverrideInputs(0, 0);
    }

  protected:
    unsigned int m_distanceToMove;
    const bool m_forward;
    bool m_stopOnCollision = false;
private:
    unsigned int m_origDistanceToMove; // used for logging - m_distanceToMove can change over lifetime of this class 
};


struct StopAction : public MoveAction
{
    StopAction()
        : MoveAction(50000, -30)
    {
        m_stopOnCollision = true;
    }
    const char* Name() override { return "StopAction"; }
};


struct MoveToPlatformAction : public MoveAction
{
    int m_diff = 0;
    int m_diffMax = 0;
    int m_diffMin = 10000; // some big number
    int m_slowCount = 0;
    bool m_fIsLow = false;
    int m_distanceFirstHit = 0;

    const char* Name() override { return "MoveToPlatformAction"; }

    MoveToPlatformAction(int distance, int angle)
        : MoveAction(distance, 85),
          m_angle(angle)
    {}

    bool ShouldStop() override
    {
        int distance = m_main.drive.m_distance * 10;
        m_diff = (m_diff + (distance - (int)m_lastDistance)) / 2;

        if (m_diffMax < m_diff)
            m_diffMax = m_diff;
        if (m_diffMin > m_diff)
            m_diffMin = m_diff;

        // ReportStatus("MoveToPlatform: diff = %d/%d, max  = %d, min = %d\n", m_diff, distance - (int)m_lastDistance, m_diffMax, m_diffMin);

        if (!m_fIsLow && m_diff <= m_diffMax - 90)
        {
            // ReportStatus("MoveToPlatform: Slow down: diff = %d/%d, max  = %d, min = %d\n", m_diff, distance - (int)m_lastDistance, m_diffMax, m_diffMin);
            m_diffMin = m_diff;
            m_fIsLow = true;
            if (m_slowCount == 0)
            {
                m_main.drive.OverrideInputs(100, 0);
                m_distanceFirstHit = distance;
            }
        }

        if (m_fIsLow && distance >= m_distanceFirstHit + 1200 * 10)
            return true;

        m_lastDistance = distance;

        if (m_main.drive.m_distance >= m_distanceToMove)
        {
            ReportStatus("MoveToPlatform: Stop based on disance!\n");
            return true;
        }
        return false;
    }

protected:
    KeepAngle m_angle;
    unsigned int m_lastDistance = 0;
};


struct MoveExactAction : public MoveAction
{
    const char* Name() override { return "MoveExactAction"; }

    MoveExactAction(int distance, int angle)
        : MoveAction(distance, 0 /*power*/),
          m_angle(angle)
    {
    }

    virtual int SpeedFromDistance(int error)
    {
        static constexpr unsigned int points[] = {30, 60, 1600, UINT_MAX};
        static constexpr unsigned int speeds[] = {0,  60, 4000, 4000};
        return SpeedFromDistances(error, points, speeds);
    }

    bool ShouldStop() override
    {
        unsigned int distance = m_main.drive.m_distance;
        int error = (int)m_distanceToMove - int(distance);

        PositionInfo info = GetTracker().LatestPosition(false /*clicks*/);
        
        // 1 tick/ms on each wheel (roughly 36"/sec - unreachable speed) == 72 in actualSpeed
        int actualSpeed = 1000 * (info.leftSpeed + info.rightSpeed);
        int idealSpeed = SpeedFromDistance(error);
        if (!m_forward)
            actualSpeed = -actualSpeed; // make it positive

        if ((idealSpeed == 0 && abs(actualSpeed) <= 18) || error < 0)
            return true;
 
        // Moving counter-clockwise: Positive means we need to speed up rotation
        // moveing clockwise: negative means we need to speed up rotation
        // Overall: positive means add to counter-clockwise direction
        // In gyro ticks per second. 256 is one degree per second
        int diff = idealSpeed - actualSpeed;

        // Power calculation. Some notes:
        // We want to have smaller impact of speed difference, to keep system stable.
        // For that reason, bigger kick is comming from
        // a) "stable" power to jeep motion going - that's fixed size (with right sign)
        // b) power proportional to ideal speed - the higher maintained speed, the more energy is needed to sustain it.
        // The rest is addressed by difference between nominal and desired speeds
        int power = 0;
        if (error < 0 || idealSpeed == 0)
            power = -18 + idealSpeed / 60 + diff / 50; // Stopping!
        else if (idealSpeed != 0)
            power = 18 + idealSpeed * (25 + idealSpeed / 500) / 2000 + diff / 70; // Moving forward

        // If robot stopped, or about to stop (and far from target), then give it a boost
        // Friction is too high for formular above to start moving with right speed - it's structured
        // to be stable around desired speed.
        if (actualSpeed == 0 || (abs(actualSpeed) < 20 && error > 50))
        {
            power += maxSpeed / 2;
        }

        // Start slowly, for better accuracy.
        // Reach full power in 1 second, 50% powet in .3 seconds
        int powerLimit = 30 + (m_main.GetTime() - m_timeStart) / 15;
        if (power > powerLimit)
            power = powerLimit;

        if (power > maxSpeed)
            power = maxSpeed;

        // ReportStatus("MoveExact: %d %d %d %d %d\n", error, actualSpeed, idealSpeed, diff, power);

        if (!m_forward)
            power = -power;
        m_power = (power + m_power) / 2;

        m_main.drive.OverrideInputs(m_power, 0);
        return false;
    }

  protected:
    KeepAngle m_angle;
    static const int maxSpeed = 100;
    int m_power = 0;
};


struct MoveExactFastAction : public MoveExactAction
{
    MoveExactFastAction(int distance, int angle)
        : MoveExactAction(distance, angle)
    {}

    int SpeedFromDistance(int error) override
    {
        // WARNING: is used by GoToCapWithBallUnderIt!
        // Be careful when changing values in here!
        // WARNING: Also used by
        //   - MoveHitWallAction (define minimal speed)
        //   - MoveFlipCapAction
        static constexpr unsigned int points[] = {50, 2000, UINT_MAX};
        static constexpr unsigned int speeds[] = {0,  4000, 4000};
        return SpeedFromDistances(error, points, speeds);
    }
};

using MoveFlipCapAction = MoveExactFastAction;


struct MoveHitWallAction : public MoveExactFastAction
{
    MoveHitWallAction(int distance, int angle)
        : MoveExactFastAction(distance, angle)
    {
    }

    int SpeedFromDistance(int error) override
    {
        // Keep going forever, untill we hit the wall...
        const unsigned int distanceToKeep = 300;
        int timeElapsed = m_main.GetTime() - m_timeStart;

        if (abs(error) <= distanceToKeep && timeElapsed >= 500)
        {
            m_stopOnCollision = true;
            m_distanceToMove = m_main.drive.m_distance + distanceToKeep;
        }

        if (timeElapsed >= 1500)
            m_stopOnCollision = true;

        return MoveExactAction::SpeedFromDistance(error);
    }
};


template<typename TMoveAction>
struct MoveExactWithLineCorrectionAction : public TMoveAction
{
    MoveExactWithLineCorrectionAction(int fullDistance, unsigned int distanceAfterLine, int angle)
        : TMoveAction(fullDistance, angle),
          m_distanceAfterLine(distanceAfterLine),
          m_angle(angle)
    {
        Assert(distanceAfterLine >= 0);
        TMoveAction::m_main.lineTrackerLeft.Reset();
        TMoveAction::m_main.lineTrackerRight.Reset();
    }

    bool ShouldStop() override
    {
        int shouldHaveTravelled = (int)TMoveAction::m_distanceToMove - (int)m_distanceAfterLine;        

        // Adjust distance based only on one line tracker going over line.
        // This might be useful in cases where second line tracker will never cross the line.
        // Like when driving to climb platform.
        // If we hit line with both trackers, we will re-calibrate distance based on that later on.
        if (!m_adjustedDistance)
        {
            unsigned int distance = 0;
            if (TMoveAction::m_main.lineTrackerLeft.HasWhiteLine(shouldHaveTravelled))
            {
                distance = TMoveAction::m_main.lineTrackerLeft.GetWhiteLineDistance(false/*pop*/);
                m_adjustedDistance = true;
            }
            if (TMoveAction::m_main.lineTrackerRight.HasWhiteLine(shouldHaveTravelled))
            {
                distance = TMoveAction::m_main.lineTrackerRight.GetWhiteLineDistance(false/*pop*/);
                m_adjustedDistance = true;
            }
            if (m_adjustedDistance)
            {
                ReportStatus("Single line correction: travelled: %d, Dist: %d -> %d\n",
                    distance, TMoveAction::m_distanceToMove - distance, m_distanceAfterLine + 50);
                if (g_leverageLineTrackers)
                    TMoveAction::m_distanceToMove = m_distanceAfterLine + 50 + distance;
            }
        }
        else if (m_fActive && TMoveAction::m_main.lineTrackerLeft.HasWhiteLine(shouldHaveTravelled) && TMoveAction::m_main.lineTrackerRight.HasWhiteLine(shouldHaveTravelled))
        {
            m_fActive = false; // ignore any other lines
            unsigned int left = TMoveAction::m_main.lineTrackerLeft.GetWhiteLineDistance(true/*pop*/);
            unsigned int right = TMoveAction::m_main.lineTrackerRight.GetWhiteLineDistance(true/*pop*/);
            unsigned int distance = (left + right) / 2;
            
            int diff = right - left;
            // if angles are flipped, then m_angle is flipped. SetAngle() will also flip angle to get to real one.
            if (TMoveAction::m_main.drive.IsXFlipped())
                diff = -diff;

            float angle = atan2(diff, DistanveBetweenLineSensors * 2); // left & right is double of distanve
            int angleI = angle * 180 / PositionTracker::Pi;

            ReportStatus("Double line correction: travelled: %d, Dist: %d -> %d, angle+: %d\n",
                distance, TMoveAction::m_distanceToMove - int(distance), m_distanceAfterLine, angleI);

            if (g_leverageLineTrackers)
            {
                TMoveAction::m_distanceToMove = m_distanceAfterLine + distance;
                TMoveAction::m_main.tracker.SetAngle(m_angle - angleI);
            }
        }

        bool res = TMoveAction::ShouldStop();
        if (res && m_fActive)
        {
            ReportStatus("Line correction did not happen! Max brightness: %d,  %d\n",
                TMoveAction::m_main.lineTrackerLeft.MinValue(), TMoveAction::m_main.lineTrackerRight.MinValue());
        }
        return res;
    }

protected:
    unsigned int m_distanceAfterLine;
    int m_angle;
    bool m_fActive = true;
    bool m_adjustedDistance = false;
};
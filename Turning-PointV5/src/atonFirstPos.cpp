#include "aton.h"
#include "atonFirstPos.h"

// WARNING:
// All coordinates and gyro-based turns are from the POV of RED (Left) position
// For Blue (right) automatic transformation happens

const unsigned int distanceTillWall = 6500;
const int angleToMoveToFlags = 4;
const int angleToShootFlags = -2;

void RunAtonFirstPos()
{
    auto &main = GetMain();
    auto timeBegin = main.GetTime();

    main.tracker.SetCoordinates({16, 60, -90});

    // async actions
    SetShooterAngle(twoFlagsShootsHighFirst, distanceFirstAton);

    //
    // knock the cone
    //
    GoToCapWithBallUnderIt(distanceToCap, distanceToCap + 300, -90);

    //
    // Turn to shoot
    //
    TurnToAngle(angleToShootFlags);

    ShootTwoBalls(distanceFirstAton);
    IntakeUp();

    // prepare for middle pole shooting
    bool highFlag = false;
    SetShooterAngle(highFlag, main.lcd.AtonClimbPlatform ? distanceFirstAtonFromPlatform : distanceFirstAtonDiagonalShot);

    //
    // Climb platform if neeed
    //
    if (main.lcd.AtonClimbPlatform)
    {
        HitLowFlagWithRecovery(distanceTillWall, 10500, 13 /*angleBack*/, angleToMoveToFlags);

        bool hasBall = main.shooter.BallStatus() == BallPresence::HasBall;
        auto time = main.GetTime() - timeBegin;
        ReportStatus("\n   Time before diagonal shot: %d, Ball: %d\n\n", time, (int)hasBall);
        bool shooting = hasBall && (time < 11000);
        if (shooting)
        {
            TurnToAngle(-26);
            ShootOneBall(highFlag, distanceFirstAtonFromPlatform);
        }
        TurnToAngle(-90);
        MoveToPlatform(main.lcd.AtonSkills, -90);

        if (shooting)
        {
            int time2 = main.GetTime() - time - timeBegin;
            if (time2 < 4000)
                printf("Took %d to shoot and climb\n", time2);
            else
                printf("!!! WARNING: Took too long (%d) to shoot and climb\n", time2);            
        }
    }
    else
    {
        HitLowFlagWithRecovery(distanceTillWall, 2800, 5 /*angleBack*/, angleToMoveToFlags);

        TurnToAngle(-48);
        ShootOneBall(highFlag, distanceFirstAtonDiagonalShot, true /*checkPresenceOfBall*/);

        // IntakeUp();
        // WaitForBall(1000);
        // SetShooterAngle(!highFlag, distanceFirstAtonDiagonalShot);

        FlipCap(2100, -400, -48);

        // ShootOneBall(false/*high*/, distanceFirstAtonDiagonalShot, true /*checkPresenceOfBall*/);
    }
}

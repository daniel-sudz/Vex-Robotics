#include "position.h"
#include "logger.h"
#include <cmath>

Encoder g_leftDriveEncoder = nullptr;
Encoder g_rightDriveEncoder = nullptr;
Encoder g_sideEncoder = nullptr;

PositionTracker::PositionTracker()
{
    g_leftDriveEncoder = encoderInit(leftDriveEncoderTopPort, leftDriveEncoderBotPort, false);
    g_rightDriveEncoder = encoderInit(rightDriveEncoderTopPort, rightDriveEncoderBotPort, false);
    g_sideEncoder = encoderInit(sideEncoderTopPort, sideEncoderBotPort, true);
    encoderReset(g_leftDriveEncoder);
    encoderReset(g_rightDriveEncoder);
    encoderReset(g_sideEncoder);

    long time = micros();
    m_gyro = ::GetGyro().Get();
    auto left = encoderGet(g_leftDriveEncoder);
    auto right = encoderGet(g_rightDriveEncoder);
    auto side = 0; //encoderGet(g_sideEncoder);
    auto shooterAngle = analogRead(shooterPreloadPoterntiometer);

    for (int i = 0; i < SamplesToTrack; i++)
    {
        m_time[i] = time + (SamplesToTrack - i) * PositionTrackingRefreshRate;

        m_sensor.leftEncoder[i] = left;
        m_sensor.rightEncoder[i] = right;
        m_sensor.sideEncoder[i] = side;
        m_sensor.gyro[i] = m_gyro;
        m_sensor.shooterAngle[i] = shooterAngle;

        m_position.leftSpeed[i] = 0;
        m_position.rightSpeed[i] = 0;
        m_position.sideSpeed[i] = 0;
        m_position.X[i] = 0;
        m_position.Y[i] = 0;
        m_position.gyroSpeed[i] = 0;
        m_position.shooterAngle[i] = 0;
    }

    m_time[0] = time;
}

template <typename T1, typename T2>
void PositionTracker::SmoothSeries(T1 *dataIn, T2 *dataSpeedOut, unsigned int initialSmoothingRange, unsigned int finalSmoothingRange)
{
    // we keep partial sum in the next out element
    T2 sum = dataSpeedOut[m_currentIndex];
    // That's full sum
    sum += dataIn[m_currentIndex];
    // store average
    T2 average = sum / (2 * initialSmoothingRange + 1);
    dataIn[Index(m_currentIndex - initialSmoothingRange)] = average;
    // create artal sum for next iteration
    sum -= dataIn[Index(m_currentIndex - 2 * initialSmoothingRange)];
    dataSpeedOut[Index(m_currentIndex + 1)] = sum;

    int middleIndex = Index(m_currentIndex - finalSmoothingRange - initialSmoothingRange);

    T2 x0 = Value(dataIn, m_currentIndex - 2 * finalSmoothingRange - initialSmoothingRange);

    T2 speed = (average - x0) / (2 * finalSmoothingRange);
    dataSpeedOut[middleIndex] = speed;
    dataSpeedOut[m_currentIndex] = speed;
}

void PositionTracker::RecalcPosition(int index, unsigned int multiplier)
{
    Assert(index == Index(index));

    double arcMoveForward = (m_position.leftSpeed[index] + m_position.rightSpeed[index]) / 2;
    double sideMove = m_position.sideSpeed[index];

    int indexPrev = Index(index - multiplier);
    if (arcMoveForward == 0 && sideMove == 0)
    {
        m_position.X[index] = m_position.X[indexPrev];
        m_position.Y[index] = m_position.Y[indexPrev];
    }
    else
    {
        if (multiplier != 1)
        {
            arcMoveForward *= multiplier;
            sideMove *= multiplier;
        }
        // Positive is counterclockwise, same as gyro
        // These 2 was of calculating angle should produce similar results, but see notes on resolution above...
        // double angle = (rightEncoder - leftEncoder) * one_by_wheelDistance;
        double angle = m_sensor.gyro[index] * GyroToRadiants;

        // Condition should be: angleDiffHalf != 0
        // But in practice,
        //     sin(pi/180)/(pi/180) = 0.99995
        //     sin(0.4*pi/180)/(0.4*pi/180) = 0.99999
        double gyroSpeed = m_position.gyroSpeed[index];
        double angleDiffHalf = gyroSpeed * GyroToRadiants / 2;
        if (abs((int)gyroSpeed) >= GyroWrapper::Multiplier / 16)
        {
            double sinDiffHalf = sin(angleDiffHalf);
            double coeff = sinDiffHalf / angleDiffHalf;
            arcMoveForward *= coeff;
            sideMove = sideMove * coeff + 2 * sinDiffHalf * distanceMiddleWheelFromCenter;
        }
        double angleHalf = angle - angleDiffHalf;
        double sinA = sin(angleHalf);
        double cosA = cos(angleHalf);
        m_position.X[index] = m_position.X[indexPrev] - sinA * arcMoveForward;
        m_position.Y[index] = m_position.Y[indexPrev] - cosA * arcMoveForward;
        if (sideMove != 0)
        {
            m_position.X[index] += -cosA * sideMove;
            m_position.Y[index] += sinA * sideMove;
        }
    }
}

// Based on http://thepilons.ca/wp-content/uploads/2018/10/Tracking.pdf
// We are missing here third wheel, so we can't account for bumps form the side.
//
// ***         ***
// ***  TURNS  ***
// ***         ***
//      Full in-place turn == 360*3.547 clicks on each wheel.
//      Or roughly 0.14 degrees per one click.
//      Gyro, on another hand, 0.004 degree resolution (or 35.5 times better than wheel resolution).
//      Max full turn in one second == 360 degree/second, measurements per 10ms:
//          - 25.5 click difference between wheels
//          - Or 256*3.6 = 921.6 clicks on gyro.
//
// ***         ***
// ***  MOVING ***
// ***         ***
//      One tick is roughly 0.9 mm
//      Assuming 24" / second speed:
//              670 clicks / second
//              6.7 clicks per 10ms
//      Assuming extreame case (which is not possible)
//          1 click / millisecond = 35" / second
//      Tthat said, we exceeed that rate when turning, as robot is not really moving (less energy spent per click)
//
void PositionTracker::Update()
{
    int leftEncoder = encoderGet(g_leftDriveEncoder);
    int rightEncoder = encoderGet(g_rightDriveEncoder);
    int sideEncoder = 0; // encoderGet(g_sideEncoder);
    m_gyro = ::GetGyro().Get();
    unsigned long time = micros();

    GetLogger().Log(LogEntry::Position,
                    leftEncoder - m_sensor.leftEncoder[m_currentIndex],
                    rightEncoder - m_sensor.rightEncoder[m_currentIndex],
                    m_gyro - m_sensor.gyro[m_currentIndex]); // sideEncoder

    int i = Index(m_currentIndex + 1);
    m_currentIndex = i;
    m_time[i] = time;
    m_sensor.leftEncoder[i] = leftEncoder;
    m_sensor.rightEncoder[i] = rightEncoder;
    m_sensor.sideEncoder[i] = sideEncoder;
    m_sensor.gyro[i] = m_gyro;
    m_sensor.shooterAngle[i] = 0;

    SmoothSeries(m_sensor.leftEncoder, m_position.leftSpeed, c_initialSmoothingRange, c_finalSmoothingRange);
    SmoothSeries(m_sensor.rightEncoder, m_position.rightSpeed, c_initialSmoothingRange, c_finalSmoothingRange);
    SmoothSeries(m_sensor.sideEncoder, m_position.sideSpeed, c_initialSmoothingRange, c_finalSmoothingRange);
    SmoothSeries(m_sensor.gyro, m_position.gyroSpeed, 1, 2);
    // SmoothSeries(m_sensor.shooterAngle, m_position.shooterAngle);

    RecalcPosition(Index(m_currentIndex - c_initialSmoothingRange - c_finalSmoothingRange), 1);
    RecalcPosition(m_currentIndex, c_initialSmoothingRange + c_finalSmoothingRange);

    m_count++;

#if 0
    const int halfCycle = 500;
    int udpateCycle = m_count % (2*halfCycle);
    if (udpateCycle == 0)
        lcdPrint(uart1, 1, "XYA %d %d %d",
            int(m_position.X[m_currentIndex] * inchesPerClick * 10), int(m_position.Y[m_currentIndex] * inchesPerClick * 10),
            int(m_sensor.gyro[m_currentIndex] * 10 / GyroWrapper::Multiplier) % 3600);
    if (udpateCycle == 1)
        lcdPrint(uart1, 2, "LRA: %d %d %d",
            encoderGet(g_leftDriveEncoder), encoderGet(g_rightDriveEncoder), int(m_gyro / GyroWrapper::Multiplier) % 360);
#endif
}

PositionInfo PositionTracker::LatestPosition(bool clicks)
{
    PositionInfo info;
    // we do not recalculate speed for latest item, only for
    int index = Index(m_currentIndex - c_initialSmoothingRange);
    info.leftSpeed = m_position.leftSpeed[index] / PositionTrackingRefreshRate;
    info.rightSpeed = m_position.rightSpeed[index] / PositionTrackingRefreshRate;
    info.gyroSpeed = m_position.gyroSpeed[index] / PositionTrackingRefreshRate;
    info.gyro = ::GetGyro().Get();

    if (clicks)
    {
        info.X = m_position.X[m_currentIndex];
        info.Y = m_position.Y[m_currentIndex];
    }
    else
    {
        info.X = m_position.X[m_currentIndex] * inchesPerClick;
        info.Y = m_position.Y[m_currentIndex] * inchesPerClick;
    }

    if (m_flipX)
    {
        info.gyroSpeed = -info.gyroSpeed;
        info.gyro = -info.gyro;
        info.X = -info.X;
    }

    return info;
}

void PositionTracker::FlipX(bool flip)
{
    m_flipX = flip;
}

int PositionTracker::GetGyro()
{
    int angle = ::GetGyro().Get();
    if (m_flipX)
        angle = -angle;
    return angle;
}

void PositionTracker::SetAngle(int degrees)
{
    degrees *= GyroWrapper::Multiplier;
    if (m_flipX)
        degrees = -degrees;
    for (int i = 0; i < SamplesToTrack; i++)
        m_sensor.gyro[i] = degrees;

    ::GetGyro().SetAngle(degrees);
}

void PositionTracker::SetCoordinates(Coordinates coord)
{
    SetAngle(coord.angle);
    coord.X /= inchesPerClick;
    coord.Y /= inchesPerClick;

    if (m_flipX)
        coord.X = -coord.X;

    for (int i = 0; i < SamplesToTrack; i++)
    {
        m_position.X[i] = coord.X;
        m_position.Y[i] = coord.Y;
    }
}

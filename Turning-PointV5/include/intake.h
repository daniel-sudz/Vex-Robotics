#pragma once
#include "main.h"

class Intake
{
  public:
    enum class Direction
    {
        None,
        Up,
        Down,
    };

    void Update();
    void UpdateIntakeFromShooter(IntakeShoterEvent event);
    void SetIntakeDirection(Direction direction);
    void ResetState();

  private:
    void SetIntakeMotor(int speed);

  private:
    Direction m_direction = Direction::None;
    bool m_doublePressed = false;
    int m_power = 0;
    
    bool m_ballGoDownState = false;


};

class Descorer
{
    enum class Direction
    {
        None,
        Up,
        Down,
    };
    int m_count = 0;
    Direction m_direction = Direction::None;

  public:
    void Update();
};

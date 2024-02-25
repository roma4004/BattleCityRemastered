#pragma once

#include "../headers/Environment.h"

class IMovable
{
public:
  virtual ~IMovable() = default;
  virtual void Move(const Environment& env) = 0;
};

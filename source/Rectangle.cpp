#include "../headers/Rectangle.h"

float Rectangle::Area() const { return w * h; }

float Rectangle::Right() const { return x + w; }

float Rectangle::Bottom() const { return y + h; }

#include "../headers/ObjRectangle.h"

float ObjRectangle::Area() const { return w * h; }

float ObjRectangle::Right() const { return x + w; }

float ObjRectangle::Bottom() const { return y + h; }

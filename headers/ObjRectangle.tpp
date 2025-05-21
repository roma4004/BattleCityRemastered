#pragma once

template<class Archive>
void ObjRectangle::serialize(Archive& ar, const unsigned int /*version*/)
{
	ar & x;
	ar & y;
	ar & w;
	ar & h;
}


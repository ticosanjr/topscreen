#include "coord.hpp"

namespace topscreen::core {

Vec2f screen_to_content(double sx, double sy, const Monitor &m)
{
	Vec2f out;
	out.x = m.width > 0.0 ? (sx - m.x) / m.width : 0.0;
	out.y = m.height > 0.0 ? (sy - m.y) / m.height : 0.0;
	return out;
}

Vec2f content_to_screen(double cu, double cv, const Monitor &m)
{
	Vec2f out;
	out.x = m.x + cu * m.width;
	out.y = m.y + cv * m.height;
	return out;
}

bool monitor_contains(const Monitor &m, double sx, double sy)
{
	return sx >= m.x && sx < m.x + m.width && sy >= m.y && sy < m.y + m.height;
}

} // namespace topscreen::core

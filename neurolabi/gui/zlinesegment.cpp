#include "zlinesegment.h"
#include "tz_utilities.h"

ZLineSegment::ZLineSegment()
{
}

ZLineSegment::ZLineSegment(
    double x0, double y0, double z0, double x1, double y1, double z1)
{
  m_start.set(x0, y0, z0);
  m_end.set(x1, y1, z1);
}

ZLineSegment::ZLineSegment(const ZPoint &v0, const ZPoint &v1)
{
  m_start = v0;
  m_end = v1;
}

void ZLineSegment::setStartPoint(double x, double y, double z)
{
  m_start.set(x, y, z);
}

void ZLineSegment::setStartPoint(const ZPoint &pt)
{
  m_start = pt;
}

void ZLineSegment::setEndPoint(double x, double y, double z)
{
  m_end.set(x, y, z);
}

void ZLineSegment::setEndPoint(const ZPoint &pt)
{
  m_end = pt;
}

void ZLineSegment::set(const ZPoint &start, const ZPoint &end)
{
  setStartPoint(start);
  setEndPoint(end);
}

void ZLineSegment::print() const
{
  std::cout << "Line segment: " << m_start.toString() << " --> "
            << m_end.toString() << std::endl;
}

double ZLineSegment::getLength() const
{
  return m_start.distanceTo(m_end);
}

void ZLineSegment::invert()
{
  ZPoint tmp;
  SWAP2(m_start, m_end, tmp);
}

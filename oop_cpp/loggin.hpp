#pragma once

#include <ostream>
#include <vector>

class SimObject;
class Simulation;

void writeTimelineLog(int time_sec,
                      const std::vector<SimObject *> &objects,
                      const Simulation &simulation,
                      std::ostream &out);

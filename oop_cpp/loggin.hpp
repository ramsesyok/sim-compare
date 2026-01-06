#pragma once

#include <ostream>
#include <vector>

class SimObject;
class Simulation;

// ログ生成はシミュレーション本体から分離し、入出力責務を集約します。
void writeTimelineLog(int time_sec,
                      const std::vector<SimObject *> &objects,
                      const Simulation &simulation,
                      std::ostream &out);

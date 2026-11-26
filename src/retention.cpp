#include "retention.h"

#include <algorithm>

namespace Retention {

std::vector<std::string> filesToDelete(std::vector<DayFileInfo> days, uint64_t freeBytes,
                                        uint64_t minFreeBytes, const std::string& currentDayFile) {
  std::vector<DayFileInfo> candidates;
  candidates.reserve(days.size());
  for (const DayFileInfo& day : days) {
    if (day.name != currentDayFile) {
      candidates.push_back(day);
    }
  }
  std::sort(candidates.begin(), candidates.end(),
            [](const DayFileInfo& a, const DayFileInfo& b) { return a.name < b.name; });

  std::vector<std::string> victims;
  size_t i = 0;
  while (freeBytes < minFreeBytes && i < candidates.size()) {
    victims.push_back(candidates[i].name);
    freeBytes += candidates[i].sizeBytes;
    ++i;
  }
  return victims;
}

}  // namespace Retention

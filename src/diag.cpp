#include "diag.h"

#include <esp_system.h>

namespace Diag {

const char* resetReasonString() {
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON:
      return "normal startup (power-on)";
    case ESP_RST_EXT:
      return "external reset pin";
    case ESP_RST_SW:
      return "software reset";
    case ESP_RST_PANIC:
      return "panic / crash";
    case ESP_RST_INT_WDT:
      return "interrupt watchdog";
    case ESP_RST_TASK_WDT:
      return "task watchdog";
    case ESP_RST_WDT:
      return "other watchdog";
    case ESP_RST_DEEPSLEEP:
      return "woke from deep sleep";
    case ESP_RST_BROWNOUT:
      return "brownout (power supply too low)";
    case ESP_RST_SDIO:
      return "reset via SDIO";
    default:
      return "unknown";
  }
}

}  // namespace Diag

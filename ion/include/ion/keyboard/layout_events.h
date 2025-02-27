#ifndef ION_KEYBOARD_LAYOUT_EVENTS_H
#define ION_KEYBOARD_LAYOUT_EVENTS_H

#include <ion/events.h>
#include <ion/unicode/code_point.h>
#include <string.h>

#include "event_data.h"

namespace Ion {
namespace Events {

extern const EventData s_dataForEvent[Event::k_specialEventsOffset];

#if DEBUG
extern const char* const s_nameForEvent[255];

inline const char* Event::name() const {
  assert(strlen(s_nameForEvent[m_id]) > 0);
  return s_nameForEvent[m_id];
}
#endif

}  // namespace Events
}  // namespace Ion

#endif

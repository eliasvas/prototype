#include "input.h"
#include "stdio.h"

// TODO: Gamepad Events
// TODO: Scrolling Events
// TODO: Multiple keyboards/mouses (Gonna be hard we have to map KB/M to logical indices I think)

void input_process_events(Input *input) {
  for (Input_Event_Node *node = input->first; node != nullptr; node = node->next) {
    Input_Event evt = node->evt;
    switch (evt.kind) {
      case INPUT_EVENT_KIND_NONE:
        // We should never have empty events in the queue
        assert(0 && "Empty event cannot be processed");
        break;
      case INPUT_EVENT_KIND_KEEB:
        Input_Keeb_Event ke = evt.data.ke;
        if (input->keeb_state[ke.scancode].is_down != ke.is_down) {
          input->keeb_state[ke.scancode].is_down = ke.is_down;
          input->keeb_state[ke.scancode].transition_count += 1;
        }
        break;
      case INPUT_EVENT_KIND_MOUSE:
        Input_Mouse_Event me = evt.data.me;
        if (input->mouse_state[me.button].is_down != me.is_down) {
          input->mouse_state[me.button].is_down = me.is_down;
          input->mouse_state[me.button].transition_count += 1;
        }
        break;
      case INPUT_EVENT_KIND_MOUSEMOTION:
        input->mouse_pos = evt.data.mme.mouse_pos; 
        break;
      case INPUT_EVENT_KIND_MOUSEWHEEL:
        input->wheel_delta = v2_add(input->wheel_delta, evt.data.mwe.wheel_delta); 
        break;
      case INPUT_EVENT_KIND_GAMEPAD:
        // TBA
        break;
      default: 
        assert(0 && "Corrupted event cannot be processed");
        break;
    }
  }
  // clear the event queue at the end
  input->first = nullptr;
  input->last = nullptr;
}

void input_end_frame(Input *input) {
  // Reset transition counts and set 'was_down' to previous frame's is_down field
  for (u32 keeb_key_idx = 0; keeb_key_idx < KEY_SCANCODE_COUNT; ++keeb_key_idx) {
    input->keeb_state[keeb_key_idx].was_down = input->keeb_state[keeb_key_idx].is_down;
    input->keeb_state[keeb_key_idx].transition_count = 0;
  }
  for (u32 mouse_key_idx = 0; mouse_key_idx < INPUT_MOUSE_COUNT; ++mouse_key_idx) {
    input->mouse_state[mouse_key_idx].was_down = input->mouse_state[mouse_key_idx].is_down;
    input->mouse_state[mouse_key_idx].transition_count = 0;
  }
  input->prev_mouse_pos = input->mouse_pos;
  input->wheel_delta = v2m(0,0);
}

void input_push_event(Input *input, Arena *arena, Input_Event *evt) {
  if (evt->kind != INPUT_EVENT_KIND_NONE) {
    Input_Event_Node *new_event = arena_push_array(arena, Input_Event_Node, 1);
    M_COPY(&new_event->evt, evt, sizeof(Input_Event));
    sll_queue_push(input->first, input->last, new_event);
  }
}

// TODO: Maybe we should also take transitions into account? (yes)

b32 input_key_pressed(Input* input, Key_Scancode key) {
  return input->keeb_state[key].is_down && !input->keeb_state[key].was_down;
}
b32 input_key_released(Input* input, Key_Scancode key) {
  return !input->keeb_state[key].is_down && input->keeb_state[key].was_down;
}
b32 input_key_up(Input *input, Key_Scancode key) {
  return !input->keeb_state[key].is_down;
}
b32 input_key_down(Input *input, Key_Scancode key) {
  return input->keeb_state[key].is_down;
}

b32 input_mkey_pressed(Input *input, Input_Mouse_Button button) {
  return input->mouse_state[button].is_down && !input->mouse_state[button].was_down;
}
b32 input_mkey_released(Input *input, Input_Mouse_Button button) {
  return !input->mouse_state[button].is_down && input->mouse_state[button].was_down;
}
b32 input_mkey_up(Input *input, Input_Mouse_Button button) {
  return !input->mouse_state[button].is_down;
}
b32 input_mkey_down(Input *input, Input_Mouse_Button button) {
  return input->mouse_state[button].is_down;
}

v2 input_get_mouse_pos(Input *input) {
  return input->mouse_pos;
}
v2 input_get_mouse_delta(Input *input) {
  return v2_sub(input->mouse_pos, input->prev_mouse_pos);
}

v2 input_get_scroll_delta(Input *input) {
  return input->wheel_delta;
}

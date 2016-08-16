#include <mruby.h>
#include <mruby/array.h>
#include <mruby/class.h>
#include <mruby/data.h>
#include <mruby/variable.h>
#include <mruby/hash.h>
#include "gamepad/Gamepad.h"

static void
gamepad_free(mrb_state *mrb, void *ptr)
{
  /* The pointer is handled by the gamepad library, we should not attempt to free it */
}

const struct mrb_data_type mrb_libstem_gamepad_type = { "Gamepad_device", gamepad_free };

#define E_GAMEPAD_ERROR             (mrb_class_get_under(mrb, mrb_module_get(mrb, "Libstem"), "GamepadError"))

static struct RClass*
gamepad_get_class(mrb_state* mrb)
{
  return mrb_class_get_under(mrb, mrb_module_get(mrb, "Libstem"), "Gamepad");
}

static mrb_value
gamepad_get_class_value(mrb_state* mrb)
{
  return mrb_obj_value(gamepad_get_class(mrb));
}

static mrb_value
mrb_gamepad_value(mrb_state* mrb, struct Gamepad_device* device)
{
  mrb_value instance = mrb_obj_new(mrb, gamepad_get_class(mrb), 0, NULL);
  mrb_data_init(instance, device, &mrb_libstem_gamepad_type);
  return instance;
}

static void
gamepad_set_mruby_callback(mrb_state* mrb, const char* name, mrb_value cb)
{
  mrb_sym sym = mrb_intern_cstr(mrb, name);
  mrb_iv_set(mrb, gamepad_get_class_value(mrb), sym, cb);
}

static mrb_value
gamepad_get_mruby_callback(mrb_state* mrb, const char* name)
{
  mrb_sym sym = mrb_intern_cstr(mrb, name);
  return mrb_iv_get(mrb, gamepad_get_class_value(mrb), sym);
}

static mrb_value
gamepad_get_cached_devices(mrb_state* mrb)
{
  mrb_value klass = gamepad_get_class_value(mrb);
  mrb_sym varname = mrb_intern_cstr(mrb, "@__cached_devices");
  mrb_value hash = mrb_iv_get(mrb, klass, varname);
  if (mrb_nil_p(hash)) {
    hash = mrb_hash_new(mrb);
    mrb_iv_set(mrb, klass, varname, hash);
  }
  return hash;
}

static mrb_value
gamepad_get_cached_device(mrb_state* mrb, struct Gamepad_device* device)
{
  if (device) {
    mrb_value key = mrb_fixnum_value(device->deviceID);
    mrb_value hash = gamepad_get_cached_devices(mrb);
    mrb_value mdev = mrb_hash_get(mrb, hash, key);
    if (mrb_nil_p(mdev)) {
      mdev = mrb_gamepad_value(mrb, device);
      mrb_hash_set(mrb, hash, key, mdev);
    }
    return mdev;
  }
  return mrb_nil_value();
}

static struct Gamepad_device*
gamepad_get_ptr(mrb_state* mrb, mrb_value self)
{
  if (mrb_bool(mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "@destroyed")))) {
    mrb_raisef(mrb, E_GAMEPAD_ERROR, "Cannot access removed device!");
    return NULL;
  }
  return (struct Gamepad_device*)mrb_data_get_ptr(mrb, self, &mrb_libstem_gamepad_type);
}

static mrb_value
gamepad_destroy_cached_device(mrb_state* mrb, struct Gamepad_device* device)
{
  if (device) {
    mrb_value key = mrb_fixnum_value(device->deviceID);
    mrb_value hash = gamepad_get_cached_devices(mrb);
    mrb_value mdev = mrb_hash_delete_key(mrb, hash, key);
    return mdev;
  }
  return mrb_nil_value();
}

/**
 * In case something went wrong
 */
static mrb_value
gamepad_m_clear_device_cache(mrb_state* mrb, mrb_value self)
{
  mrb_hash_clear(mrb, gamepad_get_cached_devices(mrb));
  return self;
}

static mrb_value
gamepad_m_init(mrb_state* mrb, mrb_value self)
{
  Gamepad_init();
  return self;
}

static mrb_value
gamepad_m_shutdown(mrb_state* mrb, mrb_value self)
{
  Gamepad_shutdown();
  return self;
}

static mrb_value
gamepad_m_num_devices(mrb_state* mrb, mrb_value self)
{
  return mrb_fixnum_value(Gamepad_numDevices());
}

static mrb_value
gamepad_m_device_at_index(mrb_state* mrb, mrb_value self)
{
  mrb_int index;
  struct Gamepad_device* device;
  mrb_get_args(mrb, "i", &index);
  device = Gamepad_deviceAtIndex(index);
  return gamepad_get_cached_device(mrb, device);
}

static mrb_value
gamepad_m_detect_devices(mrb_state* mrb, mrb_value self)
{
  Gamepad_detectDevices();
  return self;
}

static mrb_value
gamepad_m_process_events(mrb_state* mrb, mrb_value self)
{
  Gamepad_processEvents();
  return self;
}

static void
gamepad_device_attach_func_handler(struct Gamepad_device* device, void* context)
{
  mrb_state* mrb = (mrb_state*)context;
  mrb_value blk = gamepad_get_mruby_callback(mrb, "cb_device_attach_func");
  mrb_value argv[] = {
    gamepad_get_cached_device(mrb, device)
  };
  mrb_yield_argv(mrb, blk, 1, argv);
}

static void
gamepad_device_remove_func_handler(struct Gamepad_device* device, void* context)
{
  mrb_state* mrb = (mrb_state*)context;
  mrb_value blk = gamepad_get_mruby_callback(mrb, "cb_device_remove_func");
  mrb_value mdev = gamepad_destroy_cached_device(mrb, device);
  mrb_value argv[] = { mdev };
  mrb_yield_argv(mrb, blk, 1, argv);
  if (!mrb_nil_p(mdev)) {
    mrb_iv_set(mrb, mdev, mrb_intern_lit(mrb, "@destroyed"), mrb_true_value());
  }
}

static void
gamepad_button_down_func_handler(struct Gamepad_device* device, unsigned int buttonID, double timestamp, void* context)
{
  mrb_state* mrb = (mrb_state*)context;
  mrb_value blk = gamepad_get_mruby_callback(mrb, "cb_button_down_func");
  mrb_value argv[] = {
    gamepad_get_cached_device(mrb, device),
    mrb_fixnum_value(buttonID),
    mrb_float_value(mrb, timestamp)
  };
  mrb_yield_argv(mrb, blk, 3, argv);
}

static void
gamepad_button_up_func_handler(struct Gamepad_device* device, unsigned int buttonID, double timestamp, void* context)
{
  mrb_state* mrb = (mrb_state*)context;
  mrb_value blk = gamepad_get_mruby_callback(mrb, "cb_button_up_func");
  mrb_value argv[] = {
    gamepad_get_cached_device(mrb, device),
    mrb_fixnum_value(buttonID),
    mrb_float_value(mrb, timestamp)
  };
  mrb_yield_argv(mrb, blk, 3, argv);
}

static void
gamepad_axis_move_func_handler(struct Gamepad_device* device, unsigned int axisID, float value, float lastValue, double timestamp, void* context)
{
  mrb_state* mrb = (mrb_state*)context;
  mrb_value blk = gamepad_get_mruby_callback(mrb, "cb_axis_move_func");
  mrb_value argv[] = {
    gamepad_get_cached_device(mrb, device),
    mrb_fixnum_value(axisID),
    mrb_float_value(mrb, value),
    mrb_float_value(mrb, lastValue),
    mrb_float_value(mrb, timestamp)
  };
  mrb_yield_argv(mrb, blk, 5, argv);
}

static mrb_value
get_handler_block(mrb_state* mrb, mrb_value self)
{
  mrb_value blk = mrb_nil_value();
  mrb_value obj = mrb_nil_value();
  mrb_get_args(mrb, "|o&", &obj, &blk);
  if (mrb_nil_p(obj)) {
    return blk;
  }
  return obj;
}

/**
 * @yieldparam [Libstem::Gamepad] device
 */
static mrb_value
gamepad_m_set_device_attach_func(mrb_state* mrb, mrb_value self)
{
  mrb_value blk = get_handler_block(mrb, self);
  gamepad_set_mruby_callback(mrb, "cb_device_attach_func", blk);
  if (mrb_nil_p(blk)) {
    Gamepad_deviceAttachFunc(NULL, NULL);
  } else {
    Gamepad_deviceAttachFunc(gamepad_device_attach_func_handler, mrb);
  }
  return self;
}

/**
 * @yieldparam [Libstem::Gamepad] device
 */
static mrb_value
gamepad_m_set_device_remove_func(mrb_state* mrb, mrb_value self)
{
  mrb_value blk = get_handler_block(mrb, self);
  gamepad_set_mruby_callback(mrb, "cb_device_remove_func", blk);
  if (mrb_nil_p(blk)) {
    Gamepad_deviceRemoveFunc(NULL, NULL);
  } else {
    Gamepad_deviceRemoveFunc(gamepad_device_remove_func_handler, mrb);
  }
  return self;
}

/**
 * @yieldparam [Libstem::Gamepad] device
 * @yieldparam [Integer] button_id
 * @yieldparam [Float] timestamp
 */
static mrb_value
gamepad_m_set_button_down_func(mrb_state* mrb, mrb_value self)
{
  mrb_value blk = get_handler_block(mrb, self);
  gamepad_set_mruby_callback(mrb, "cb_button_down_func", blk);
  if (mrb_nil_p(blk)) {
    Gamepad_buttonDownFunc(NULL, NULL);
  } else {
    Gamepad_buttonDownFunc(gamepad_button_down_func_handler, mrb);
  }
  return self;
}

/**
 * @yieldparam [Libstem::Gamepad] device
 * @yieldparam [Integer] button_id
 * @yieldparam [Float] timestamp
 */
static mrb_value
gamepad_m_set_button_up_func(mrb_state* mrb, mrb_value self)
{
  mrb_value blk = get_handler_block(mrb, self);
  gamepad_set_mruby_callback(mrb, "cb_button_up_func", blk);
  if (mrb_nil_p(blk)) {
    Gamepad_buttonUpFunc(NULL, NULL);
  } else {
    Gamepad_buttonUpFunc(gamepad_button_up_func_handler, mrb);
  }
  return self;
}

/**
 * @yieldparam [Libstem::Gamepad] device
 * @yieldparam [Integer] axis_id
 * @yieldparam [Float] value
 * @yieldparam [Float] last_value
 * @yieldparam [Float] timestamp
 */
static mrb_value
gamepad_m_set_axis_move_func(mrb_state* mrb, mrb_value self)
{
  mrb_value blk = get_handler_block(mrb, self);
  gamepad_set_mruby_callback(mrb, "cb_axis_move_func", blk);
  if (mrb_nil_p(blk)) {
    Gamepad_axisMoveFunc(NULL, NULL);
  } else {
    Gamepad_axisMoveFunc(gamepad_axis_move_func_handler, mrb);
  }
  return self;
}

/**
 * @return [Integer] device_id
 */
static mrb_value
gamepad_get_device_id(mrb_state* mrb, mrb_value self)
{
  return mrb_fixnum_value(gamepad_get_ptr(mrb, self)->deviceID);
}

/**
 * @return [String] description
 */
static mrb_value
gamepad_get_description(mrb_state* mrb, mrb_value self)
{
  return mrb_str_new_cstr(mrb, gamepad_get_ptr(mrb, self)->description);
}

/**
 * @return [Integer] vendor_id
 */
static mrb_value
gamepad_get_vendor_id(mrb_state* mrb, mrb_value self)
{
  return mrb_fixnum_value(gamepad_get_ptr(mrb, self)->vendorID);
}

/**
 * @return [Integer] product_id
 */
static mrb_value
gamepad_get_product_id(mrb_state* mrb, mrb_value self)
{
  return mrb_fixnum_value(gamepad_get_ptr(mrb, self)->productID);
}

/**
 * @return [Integer] axes
 */
static mrb_value
gamepad_get_num_axes(mrb_state* mrb, mrb_value self)
{
  return mrb_fixnum_value(gamepad_get_ptr(mrb, self)->numAxes);
}

/**
 * @return [Integer] buttons
 */
static mrb_value
gamepad_get_num_buttons(mrb_state* mrb, mrb_value self)
{
  return mrb_fixnum_value(gamepad_get_ptr(mrb, self)->numButtons);
}

/**
 * @return [Array<Float>] states
 */
static mrb_value
gamepad_get_axis_states(mrb_state* mrb, mrb_value self)
{
  struct Gamepad_device* device = gamepad_get_ptr(mrb, self);
  mrb_value ary = mrb_ary_new_capa(mrb, device->numAxes);
  for (unsigned int i = 0; i < device->numAxes; ++i) {
    mrb_ary_set(mrb, ary, i, mrb_float_value(mrb, device->axisStates[i]));
  }
  return ary;
}

/**
 * @return [Array<Boolean>] states
 */
static mrb_value
gamepad_get_button_states(mrb_state* mrb, mrb_value self)
{
  struct Gamepad_device* device = gamepad_get_ptr(mrb, self);
  mrb_value ary = mrb_ary_new_capa(mrb, device->numButtons);
  for (unsigned int i = 0; i < device->numButtons; ++i) {
    mrb_ary_set(mrb, ary, i, mrb_bool_value(device->buttonStates[i]));
  }
  return ary;
}

/**
 * @param [Integer] index
 * @return [Float] state
 */
static mrb_value
gamepad_get_axis_state(mrb_state* mrb, mrb_value self)
{
  mrb_int index;
  struct Gamepad_device* device;
  mrb_get_args(mrb, "i", &index);
  device = gamepad_get_ptr(mrb, self);
  if (index < 0 || index >= (mrb_int)device->numAxes) {
    mrb_raisef(mrb, E_ARGUMENT_ERROR, "axis index is out of range");
    return mrb_nil_value();
  }
  return mrb_float_value(mrb, device->axisStates[index]);
}

/**
 * @param [Integer] index
 * @return [Boolean] state
 */
static mrb_value
gamepad_get_button_state(mrb_state* mrb, mrb_value self)
{
  mrb_int index;
  struct Gamepad_device* device;
  mrb_get_args(mrb, "i", &index);
  device = gamepad_get_ptr(mrb, self);
  if (index < 0 || index >= (mrb_int)device->numButtons) {
    mrb_raisef(mrb, E_ARGUMENT_ERROR, "button index is out of range");
    return mrb_nil_value();
  }
  return mrb_bool_value(device->buttonStates[index]);
}

void
mrb_libstem_gamepad_gamepad_init(mrb_state* mrb, struct RClass* mod)
{
  struct RClass* gamepad_cls = mrb_define_class_under(mrb, mod, "Gamepad", mrb->object_class);
  MRB_SET_INSTANCE_TT(gamepad_cls, MRB_TT_DATA);
  mrb_define_class_under(mrb, mod, "GamepadError", mrb->eStandardError_class);
  mrb_define_class_method(mrb, gamepad_cls, "clear_device_cache",     gamepad_m_clear_device_cache,     MRB_ARGS_NONE());
  mrb_define_class_method(mrb, gamepad_cls, "init",                   gamepad_m_init,                   MRB_ARGS_NONE());
  mrb_define_class_method(mrb, gamepad_cls, "shutdown",               gamepad_m_shutdown,               MRB_ARGS_NONE());
  mrb_define_class_method(mrb, gamepad_cls, "num_devices",            gamepad_m_num_devices,            MRB_ARGS_NONE());
  mrb_define_class_method(mrb, gamepad_cls, "device_at_index",        gamepad_m_device_at_index,        MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, gamepad_cls, "detect_devices",         gamepad_m_detect_devices,         MRB_ARGS_NONE());
  mrb_define_class_method(mrb, gamepad_cls, "process_events",         gamepad_m_process_events,         MRB_ARGS_NONE());
  mrb_define_class_method(mrb, gamepad_cls, "set_device_attach_func", gamepad_m_set_device_attach_func, MRB_ARGS_ARG(0,1) | MRB_ARGS_BLOCK());
  mrb_define_class_method(mrb, gamepad_cls, "set_device_remove_func", gamepad_m_set_device_remove_func, MRB_ARGS_ARG(0,1) | MRB_ARGS_BLOCK());
  mrb_define_class_method(mrb, gamepad_cls, "set_button_down_func",   gamepad_m_set_button_down_func,   MRB_ARGS_ARG(0,1) | MRB_ARGS_BLOCK());
  mrb_define_class_method(mrb, gamepad_cls, "set_button_up_func",     gamepad_m_set_button_up_func,     MRB_ARGS_ARG(0,1) | MRB_ARGS_BLOCK());
  mrb_define_class_method(mrb, gamepad_cls, "set_axis_move_func",     gamepad_m_set_axis_move_func,     MRB_ARGS_ARG(0,1) | MRB_ARGS_BLOCK());
  mrb_define_method(mrb, gamepad_cls, "device_id",      gamepad_get_device_id,     MRB_ARGS_NONE());
  mrb_define_method(mrb, gamepad_cls, "description",    gamepad_get_description,   MRB_ARGS_NONE());
  mrb_define_method(mrb, gamepad_cls, "vendor_id",      gamepad_get_vendor_id,     MRB_ARGS_NONE());
  mrb_define_method(mrb, gamepad_cls, "product_id",     gamepad_get_product_id,    MRB_ARGS_NONE());
  mrb_define_method(mrb, gamepad_cls, "num_axes",       gamepad_get_num_axes,      MRB_ARGS_NONE());
  mrb_define_method(mrb, gamepad_cls, "num_buttons",    gamepad_get_num_buttons,   MRB_ARGS_NONE());
  mrb_define_method(mrb, gamepad_cls, "axis_states",    gamepad_get_axis_states,   MRB_ARGS_NONE());
  mrb_define_method(mrb, gamepad_cls, "button_states",  gamepad_get_button_states, MRB_ARGS_NONE());
  // utility
  mrb_define_method(mrb, gamepad_cls, "axis_state",     gamepad_get_axis_state,    MRB_ARGS_REQ(1));
  mrb_define_method(mrb, gamepad_cls, "button_state",   gamepad_get_button_state,  MRB_ARGS_REQ(1));
}

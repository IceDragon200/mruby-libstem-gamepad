#include <mruby.h>
#include <mruby/class.h>

extern void mrb_libstem_gamepad_gamepad_init(mrb_state* mrb, struct RClass* mod);

void
mrb_mruby_libstem_gamepad_gem_init(mrb_state* mrb)
{
  struct RClass* libstem_module = mrb_define_module(mrb, "Libstem");
  mrb_libstem_gamepad_gamepad_init(mrb, libstem_module);
}

void
mrb_mruby_libstem_gamepad_gem_final(mrb_state* mrb)
{
}

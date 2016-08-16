# mruby-libstem-gamepad

## Instructions
You must link against libstem_gamepad manually, including any depedencies it may require for your particular platform

## Usage
```ruby
Libstem::Gamepad.init

Libstem::Gamepad.set_device_attach_func do |device|
  # A new attaced device
  # the device obtained is a cached value
  puts "#{device.description} entered the arena"
end

Libstem::Gamepad.set_device_remove_func do |device|
  # the device obtained was removed from the device cache
  puts "#{device.description} has left the arena"
end

Libstem::Gamepad.set_button_down_func do |device, button_id, timestamp|
  # the device obtained is a cached value
end

Libstem::Gamepad.set_button_up_func do |device, button_id, timestamp|
  # the device obtained is a cached value
end

Libstem::Gamepad.set_axis_move_func do |device, axis_id, value, last_value, timestamp|
  # the device obtained is a cached value
end
# do stuff with gamepad

# you can unset a func by passing nil or passing in no parameters when calling the method
Libstem::Gamepad.set_device_attach_func
Libstem::Gamepad.set_device_remove_func nil
Libstem::Gamepad.set_button_down_func nil
Libstem::Gamepad.set_button_up_func nil
Libstem::Gamepad.set_axis_move_func

Libstem::Gamepad.shutdown # optional, as stated in the Gamepad.h, the resources will be cleaned up
```

## Gotchas
If you set a `device_attach_func`, you should also set the `device_remove_func`, otherwise you'll experience a memory leak, everytime a device is removed.
Since the devices are cached internally and rely on the `device_remove_func` to cleanup.
In this case `Libstem::Gamepad.clear_device_cache` is provided to clear the device cache, be sure to discard any external references

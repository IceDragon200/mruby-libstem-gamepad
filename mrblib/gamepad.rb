module Libstem
  class Gamepad
    def initialize
      @destroyed = false
    end

    # @return [Boolean] was the controller detached or removed?
    def destroyed?
      @destroyed
    end

    # (see Libstem::Gamepad.num_devices)
    def self.count
      num_devices
    end

    # (see Libstem::Gamepad.process_events)
    def self.poll_events
      process_events
    end

    class << self
      alias :shutdown_wo_clearing_cache :shutdown
      def shutdown
        @__cached_devices.each do |key, value|
          value.instance_variable_set("@destroyed", true)
        end
        @__cached_devices.clear
        shutdown_wo_clearing_cache
      end
    end
  end
end

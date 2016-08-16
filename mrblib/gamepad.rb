module Libstem
  class Gamepad
    def initialize
      @destroyed = false
    end

    def destroyed?
      @destroyed
    end

    def self.count
      num_devices
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

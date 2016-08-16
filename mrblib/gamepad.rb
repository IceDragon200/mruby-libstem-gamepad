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
  end
end

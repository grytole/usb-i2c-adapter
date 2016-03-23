-------------------------------------
-- i2c = require("i2c")
-- isok = i2c.open("/dev/ttyUSB0")
-- size = i2c.write(address, register, string)
-- string = i2c.read(address, register, size)
-- i2c.close()
-------------------------------------

local I2C
do
  local rs232 = require("luars232")
  local adapter = nil
  local timeout = 100

  -- isok = i2c.open([path])
  local open = function(path)
    if not adapter then
      local path = path or "/dev/ttyUSB0"
      local err, port = rs232.open(path)
      if err ~= rs232.RS232_ERR_NOERROR then
        return false
      end
      port:set_baud_rate(rs232.RS232_BAUD_115200)
      port:set_data_bits(rs232.RS232_DATA_8)
      port:set_parity(rs232.RS232_PARITY_NONE)
      port:set_stop_bits(rs232.RS232_STOP_1)
      port:set_flow_control(rs232.RS232_FLOW_OFF)
      adapter = port
    end
    return true
  end
  
  -- size = i2c.write(address, register, string)
  local write = function(adr, reg, str)
    if adapter then
      local err, val, len
      local req = string.char(0x00, adr, reg, #str) .. str
      err, len = adapter:write(req, timeout)
      if err == rs232.RS232_ERR_NOERROR then
        err, val, len = adapter:read(1, timeout, 1)
        if err == rs232.RS232_ERR_NOERROR then
          err, val, len = adapter:read(val:byte(1), timeout, 1)
          if err == rs232.RS232_ERR_NOERROR then
            return val:byte(1)
          end
        end
      end
    end
  end
  
  -- string = i2c.read(address, register, [size])
  local read = function(adr, reg, num)
    if adapter then
      local num = num or 1
      local err, val, len
      local req = string.char(0x01, adr, reg, num)
      err, len = adapter:write(req, timeout)
      if err == rs232.RS232_ERR_NOERROR then
        err, val, len = adapter:read(1, timeout, 1)
        if err == rs232.RS232_ERR_NOERROR then
          err, val, len = adapter:read(val:byte(1), timeout, 1)
          if err == rs232.RS232_ERR_NOERROR then
            return val
          end
        end
      end
    end
    return nil
  end

  -- i2c.close()
  local close = function()
    if adapter then
      adapter:close()
      adapter = nil
    end
  end
  
  I2C = {
    open = open,
    write = write,
    read = read,
    close = close,
  }
end
return I2C

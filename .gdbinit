
set remote noack-packet off

define hook-load
  monitor reset
  monitor halt
end

define hookpost-load
  monitor reset
  monitor halt
end
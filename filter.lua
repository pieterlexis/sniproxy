local filter = require 'pdns-platform-filter/sniproxy'

function preconnect(remote, name)
  return filter.go(remote, name)
end


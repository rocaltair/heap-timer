l = require "lhtimer"

loop = l.new_mgr()

for i = 1, 5 do
	local timer = l.new_timer(loop)
	timer:start(5, 1000, function(timer)
		timer:stop()
		print("hello", timer)
	end)
end

while true do
	loop:perform()
	-- collectgarbage("collect")
	local ms = loop:next_timeout()
	l.sleep(ms)
end



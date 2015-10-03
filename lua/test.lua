require "sax"
require "string"
require "math"

local a = sax.word.new({10.3, 7, 1, -5, -5, 7.2}, 2, 8)
local b = sax.word.new("FC", 8)
assert(a == b, "Expected array to be equal to it's sax representation after transform")


local a = sax.word.new({10.3, 7, 1, -5, -5, 7.2}, 2, 8)
local values = {-9, -8, -7, -5, -5, 7.2}
local b = sax.window.new(#values, 2, 8)
for i=1,#values do b:add(values[i]) end
local d = sax.mindist(a, b)
local expected = 1.560325
assert(math.abs(expected - d) < 1e-5, string.format("Expected mindist %f, got %f", expected, d))


local window = sax.window.new(4, 2, 4)
local values = {1, 2, 3, 10.1}
local a = sax.word.new(values, 2, 4)

for i=1,4 do assert(window:add(values[i]) == true, "window:add failed") end

assert(a == window, "__eq word vs window failed")
assert(window:add({-10, 1, 2, 3, 10.1}), "more-than-n-append failed")
assert(a == window, "__eq word vs window failed")


local window = sax.window.new(4, 2, 4)
local values = {1, 2, 3, 10.1}

for i=1,4 do window:add(values[i]) end

window:clear()
assert(window == sax.word.new("##", 4))

local window = sax.window.new(4, 2, 4)
local values = {1, 2, 3, 10.1}
local a, b

for i=1,4 do window:add(values[i]) end

b = sax.word.new(values, 2, 4)

assert(window:get_word() == b, "word vs word __eq failed")
assert(b == window, "word vs window __eq failed")
assert(window == b, "word vs window __eq failed")

local window = sax.window.new(4, 2, 4)
for i=1,5 do 
    window:add({}) 
    assert(window == sax.word.new("##", 4))
end
window:add({1, 2, 3, 4})
local a = window:get_word()
for i=1,5 do 
    window:add({})
    assert(a == window)
end

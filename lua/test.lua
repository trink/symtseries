require "sax"
require "string"
require "math"

assert(sax.version() == "0.4.0", sax.version())

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

for i=1,4 do window:add(values[i]) end

assert(a == window, "__eq word vs window failed")
window:add({-10, 1, 2, 3, 10.1}) -- append more than n
assert(a == window, "__eq word vs window failed")


local window = sax.window.new(4, 2, 4)
window:add(23)
assert(tostring(window) == "#C", "received: " .. tostring(window))
local values = {1, 2, 3, 10.1}
for i=1,4 do window:add(values[i]) end
assert(tostring(window) == "AD", "received: " .. tostring(window))
window:clear()
assert(window == sax.word.new("##", 4))
window:add({23})
assert(tostring(window) == "#C", "received: " .. tostring(window))

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

local errors = {
    function() local sw = sax.window.new() end, -- new() incorrect # args
    function() local sw = sax.window.new(nil, 2, 2) end, -- invalid parameters types
    function() local sw = sax.window.new(2, nil, 2) end,
    function() local sw = sax.window.new(2, 2, nil) end,
    function() local sw = sax.window.new(9, 3, 5, nil) end,
    function() local sw = sax.window.new(1, 3, 3) end, -- out of bounds parameters
    function() local sw = sax.window.new(9, 1, 3) end,
    function() local sw = sax.window.new(9, 3, 1) end,
    function() local sw = sax.window.new(9, 3, 17) end,
    function() local sw = sax.window.new(5000, 5, 5) end,
    function() local sw = sax.window.new(10, 3, 3) end, -- n not equally divisible by w
    function() local sw = sax.word.new() end, -- new() incorrect # args
    function() local sw = sax.word.new(nil, 5) end, -- invalid parameter types
    function() local sw = sax.word.new("AAABF") end,
    function() local sw = sax.word.new("AAABF", "5") end,
    function() local sw = sax.word.new("AAABF", 6, nil) end,
    function() local sw = sax.word.new({}) end,
    function() local sw = sax.word.new({}, nil, nil, nil) end,
    function() local sw = sax.word.new("", 5) end, -- out of bounds parameters
    function() local sw = sax.word.new("AAABF", 5) end,
    function() local sw = sax.word.new("aaabc", 5) end,
    function() local sw = sax.word.new("AABBC", 1) end,
    function() local sw = sax.word.new("AABBC", 17) end,
    function() local sw = sax.word.new({}, 1, 5) end,
    function() local sw = sax.word.new({1, 2, 3}, 1, 5) end,
    function() local sw = sax.word.new(data, 3, 1) end,
    function() local sw = sax.word.new(data, 3, 17) end,
    function() local sw = sax.word.new(data, 4, 5) end,
    function() sax.mindist() end, -- invalid parameter types
    function() sax.mindist(1, 1) end,
    function() sax.mindist(word, 1) end,
    function() sax.mindist(1, word) end,
    function(win) win:add() end,
    function(win) win:add("a") end,
    function(win) win:add({1, "a"}) end,
    function(win) win.add(w1, 1) end,
    function(win) win.get_word(w1) end,
    function(win) win.clear(w1) end,
}

local function test_errors()
    win = sax.window.new(9, 3, 5)
    assert("###" == tostring(win))
    for i, v in ipairs(errors) do
        local ok, err = pcall(v, win)
        if ok then error(string.format("error test %d failed\n", i)) end
    end
end

test_errors()

local function test_nan_inf()
    local SAX_N = 9
    local SAX_W = 3
    local SAX_C = 5

    local w = sax.word.new({1/0, 1/0, 1/0,
        math.huge, math.huge, math.huge,
        -1/0, -1/0, -1/0}, SAX_W, SAX_C)
    assert("EEA" == tostring(w), "received: " .. tostring(w))

    w = sax.word.new({1/0, 1/0, 5,
        0, 0, 0,
        -1/0, -1/0, -5}, SAX_W, SAX_C)
    assert("ECA" == tostring(w), "received: " .. tostring(w))

    w = sax.word.new({1/0, 1/0, 1/0,
        0, 0, 0,
        -1/0, -1/0, -1/0}, SAX_W, SAX_C)
    assert("ECA" == tostring(w), "received: " .. tostring(w))

    w = sax.word.new({0/0, 5, 5,
        0, 0, 0,
        0/0, -5, -5}, SAX_W, SAX_C)
    assert("ECA" == tostring(w), "received: " .. tostring(w))

    w = sax.word.new({0/0, 1/0, 1/0,
        0, 0, 0,
        0/0, -1/0, -1/0}, SAX_W, SAX_C)
    assert("ECA" == tostring(w), "received: " .. tostring(w))

    w = sax.word.new({1/0, 1/0, 5,
        0/0, 0/0, 0/0,
        -1/0, -1/0, -5}, SAX_W, SAX_C)
    assert("E#A" == tostring(w), "received: " .. tostring(w))
    local w1 = sax.word.new("ECA", SAX_C)
    assert(0 ~= sax.mindist(w, w1))
    assert(w ~= w1)

    w = sax.word.new({0/0, 0/0, 0/0,
        0/0, 0/0, 0/0,
        0/0, 0/0, 0/0}, SAX_W, SAX_C)
    assert("###" == tostring(w), "received: " .. tostring(w))

    w = sax.word.new({0/0, math.huge, 0/0,}, 3, SAX_C)
    assert("#E#" == tostring(w), "received: " .. tostring(w))
    w1 = sax.word.new("#E#", SAX_C)
    assert(0 == sax.mindist(w, w1))
    assert(w == w1)

    w = sax.word.new("####", 4)
    w1 = sax.word.new("####", "4")
    assert(sax.mindist(w, w1) == 0, "received: " .. tostring(sax.mindist(w, w1)))

    w = sax.word.new("ABCD", 4)
    w1 = sax.word.new("####", 4) -- distance should be from DDAA
    local expected = sax.mindist(w, sax.word.new("DDAA", 4))
    local d = sax.mindist(w, w1)
    assert(d == expected, string.format("Expected mindist %f, got %f", expected, d))
    -- flip
    local d = sax.mindist(w1, w)
    assert(d == expected, string.format("Expected mindist %f, got %f", expected, d))

    w = sax.word.new("CCCCC", 5)
    w1 = sax.word.new("#####", 5) -- distance should be from AAAAA
    local expected = sax.mindist(w, sax.word.new("AAAAA", 5))
    local d, a, b = sax.mindist(w, w1)
    assert(d == expected, string.format("Expected mindist %f, got %f", expected, d))
    assert(a == expected, string.format("Expected mindist %f, got %f", expected, a))
    assert(b == 0, string.format("Expected mindist %f, got %f", 0, b))

    w = sax.word.new("ABDD", 4)
    w1 = sax.word.new("DBAA", 4)
    local d, a, b = sax.mindist(w, w1)
    local ed, ea, eb = 2.336536, 1.907774, 1.349000
    assert(math.abs(ed - d) < 1e-5, string.format("Expected mindist %f, got %f", ed, d))
    assert(math.abs(ea - a) < 1e-5, string.format("Expected mindist %f, got %f", ea, a))
    assert(math.abs(eb - b) < 1e-5, string.format("Expected mindist %f, got %f", eb, b))
end

test_nan_inf()

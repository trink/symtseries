#Symbolic time series representations
Means of representing your time series data in a convenient way for your task.

##Overview
The codebase under this repository is supposed to be operated in one of three ways:  

* As a standalone C library (builds automatically)
* As a standalone Lua library (builds automatically provided Lua is found on your system)
* As a plugin lib for [mozilla-services/lua_sandbox](https://github.com/mozilla-services/lua_sandbox) (is built upon fetch from main repo, linked against luasb headers)

##Installation
###Prerequisites
* C compiler (GCC 4.7+)
* Lua 5.1 (optional, needed to build Lua part)
* [CMake (3.0+)](http://cmake.org/cmake/resources/software.html)

###CMake Build Instructions
    git clone https://github.com/Quadrocube/symtseries.git
    cd symtseries
    mkdir release
    cd release
    cmake .. -DCMAKE_BUILD_TYPE=Release && make
    ctest

##SAX (Symbolic Aggregate approXimation)
###Latest SAX paper 
[iSAX 2.0](http://www.cs.ucr.edu/~eamonn/iSAX_2.0.pdf "iSAX 2.0")

###Overview
SAX is the method of time series data representation which provides a user with several interesting capabilities:  

* Shape-based pattern matching (even for HUGE pattern collections -- which is yet TBD though)
* Shape-based time series clustering
* Numerosity and dimensionality reduction of your time series data

###Example Usage

###API functions
#### window.new(n, w, c)
```lua
require "sax"
local window = sax.window.new(150, 10, 8)
```

Import _sax_ module via the Lua 'require' function. The module is
globally registered and returned by the require function. 

*Arguments*  

- n (unsigned) The number of values to keep track of (must be > 1)
- w (unsigned) The number of frames to split the window into (must be > 1 and a divisor of n)
- c (unsigned) The cardinality of the word (must be between 2 and STS_MAX_CARDINALITY)

*Return*  

- mozsvc.sax.window userdata object.

#### word.new[(v, n, w, c), (s, c)]
```lua
local a = sax.word.new({10.3, 7, 1, -5, -5, 7.2}, 2, 8)
local b = sax.word.new("FC", 8)
print(a == b)
-- prints true
```

*Arguments*  

- v (table-array) Series to be represented in SAX notation
- w (unsigned) The number of frames to split the series into (must be > 1 and a divisor of #v)
- c (unsigned) The cardinality of the word (must be between 2 and STS_MAX_CARDINALITY)  

*OR*  

- s (string) SAX-notation string denoting a word
- c (unsigned) The cardinality of the word (must be between 2 and STS_MAX_CARDINALITY)  

*Return*  

- mozsvc.sax.word userdata object.

#### mindist(a, b)
```lua
local a = sax.word.new({10.3, 7, 1, -5, -5, 7.2}, 2, 8)
local values = {-9, -8, -7, -5, -5, 7.2}
local b = sax.window.new(#values, 2, 8)
for i=1,#values do b:add(values[i]) end
local d = sax.mindist(a, b)
-- d == 1.5676734353812
```

*Arguments*  

- a, b (mozsvc.sax.word or mozsvc.sax.window) SAX words or windows to compute mindist. 
- Note that currently mindist between different-[nwc] words is not supported

*Return*  

- Lowerbounding approximation of the Euclidian distance between series represented in a and b

#### version()
```lua
print(sax.version())
```

*Return*

- Version number as a string

###Window methods

#### add(val)
```lua
local window = sax.window.new(4, 2, 4)
local values = {1, 2, 3, 10.1}
local a = sax.word.new(values, 2, 4)

for i=1,4 do print(window:add(values[i])) end

print(a == window)
print(window:add({-10, 1, 2, 3, 10.1})) -- it only copies n last values if given more than n
print(a == window)

-- prints false false false true true true
```

*Arguments*  

- val (number or array) value(s) to be appended to a window

*Return*  

- false if there's less than n values inside window after appending val or true otherwise. 

#### get_word()

*Return*  

- returns copy of current word or nil if there isn't yet enough data to construct the whole world

#### clear()
```lua
local window = sax.window.new(4, 2, 4)
local values = {1, 2, 3, 10.1}

for i=1,4 do window:add(values[i]) end

window:clear()
print(window:add(1.3))
print(window:get_word() == nil)

-- prints false true
```

*Return*  

- Nothing. Just resets the window.

#### __tostring
```lua
local win = sax.window.new(4, 2, 4)
for i=1,4 do win:add(1.5) end
print(win)
-- prints CC
```

*Return*  

- string representing the current word in window in SAX notation, in case word is incomplete, prints nil

#### __eq
```lua
local window = sax.window.new(4, 2, 4)
local values = {1, 2, 3, 10.1}
local a, b

for i=1,4 do window:add(values[i]) end

b = sax.word.new(values, 2, 4)

print(window:get_word() == b)
print(window == b)
-- prints true true
```

*Return*  

- Whether or not two words are considered equal (per-symbol, w, and c comparison)

###Word methods

#### __tostring
```lua
local a = sax.word.new({10.3, 7, 1, -5, -5, 7.2}, 2, 8)
print(a)
-- prints FC
```

*Return*  

- string representing this word in SAX notation

#### __eq
See window.__eq

#### copy()

*Return*  

- copy of mozsvc.sax.word

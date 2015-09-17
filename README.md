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
* Lua 5.1, Lua 5.2, or LuaJIT (optional, needed to build Lua part)
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
#### new_window(n, w, c)
```lua
require "sax"
local window = sax.new_window(150, 10, 8)
```

Import _sax_ module via the Lua 'require' function. The module is
globally registered and returned by the require function. 

*Arguments*  

- n (unsigned) The number of values to keep track of (must be > 1)
- w (unsigned) The number of frames to split the window into (must be > 1 and a divisor of n)
- c (unsigned) The cardinality of the word (must be between 2 and STS_MAX_CARDINALITY)

*Return*  

- mozsvc.sax.window userdata object.

#### new_word[(v, n, w, c), (s, c)]
```lua
local a = sax.new_word({10.3, 7, 1, -5, -5, 7.2}, 2, 8)
local b = sax.new_word("FC", 8)
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
local a = sax.new_word({10.3, 7, 1, -5, -5, 7.2}, 2, 8)
local values = {-9, -8, -7, -5, -5, 7.2}
local b = sax.new_window(#values, 2, 8)
for i=1,#values do b(values[i]) end
local d = sax.mindist(a, b)
-- d == 1.5676734353812
```

*Arguments*  

- a, b (mozsvc.sax.word or mozsvc.sax.window) SAX words or windows to compute mindist. 
- Note that currently mindist between different-[nwc] words is not supported

*Return*  

- Lowerbounding approximation of the Euclidian distance between series represented in a and b

###Window methods

#### add(val)
```lua
local window = sax.new_window(4, 2, 4)
local values = {1, 2, 3, 10.1}
local a = sax.new_word(values, 2, 4)
local b

for i=1,4 do print((b = window:add(values[i])) != nil) end

print(sax.mindist(a, b))

-- prints false false false true 0.0
```

*Arguments*  

- val (number) value to be appended to a window

*Return*  

- nil if there's less than n values inside window after appending val or mozsvc.sax.word representation of series in window otherwise. Note that the returned word is static to window, use word.copy if you want to store it.

#### clear()
```lua
local window = sax.new_window(4, 2, 4)
local values = {1, 2, 3, 10.1}

for i=1,4 do window:add(values[i]) end

window.clear()
print(window:add(1.3) == nil)

-- prints true
```

*Return*  

- Nothing. Just resets the window.

###Word methods

#### __tostring
```lua
local a = sax.new_word({10.3, 7, 1, -5, -5, 7.2}, 2, 8)
print(a)
-- prints FC
```

*Return*  

- string representing this word in SAX notation

#### __eq
```lua
local window = sax.new_window(4, 2, 4)
local values = {1, 2, 3, 10.1}
local a, b

for i=1,4 do a = window:add(values[i]) end

b = sax.new_word(values, 2, 4)

print(a == b)
-- prints true
```

*Return*  

- Whether or not two words are considered equal (per-symbol, w, and c comparison)

#### copy()
```lua
local window = sax.new_window(4, 2, 4)
local values = {1, 2, 3, 10.1}
local a, b

for i=1,4 do a = window:add(values[i]) end

b = a:copy()
window:add(-12)

print(a:to_string())
print(b:to_string())

-- prints CB AD
```

*Return*  

- mozsvc.sax.word

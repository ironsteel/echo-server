# A simple Echo server which handles multiple connections

## How to build

Only linux is supported, to build run

```
cmake . -DCMAKE_BUILD_TYPE=[Release|Debug]
make
```

you need a recent gcc/g++ version that supports c++17

## Running example

```
./echo-server 0.0.0.0:7777
```

on another terminal:

```
netcat localhost 7777
```

## Drawbacks and bugs

TODO: detecting "stats" command is buggy, we need to read the whole data
from the socket until we encounter '\n', otherwise any message ending
with "stats\n" will be considered a stats command

TODO: calls to write() are not handled propertly, If the message is too
big, write() can fail to write the whole data. So we need to account for
that and mark what part of the buffer was sent and what's left to be
send

No unit tests... :(

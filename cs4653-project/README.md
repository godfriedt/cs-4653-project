# Design notes
## Game architecture
The gameplay code uses two main design patterns, [entity component system](https://en.wikipedia.org/wiki/Entity_component_system)
and event queues. Information about game objects is split into seperate arrays
for each property (card face values, positions, rotations, etc). Each
source file owns one or more of these arrays, and exposes some functions to
modify their contents.

Changes to the game state happen by adding an event to a queue. There are two
queues, one for animations and one for game state. Every frame, both queues will
execute the next event if there is one, and remove the event if it has finished.
Events may add one or more new events to the queue when triggered, effectively creating a
[finite state machine](https://en.wikipedia.org/wiki/Finite-state_machine).

Each game phase event will add the first player's turn event to the queue. Each
player turn event will add the next player's turn event to the queue, or the
next game phase event if it is the last player. Other events will exist to wait
for player input, place bets, fold, etc. Game state events should form a loop
that repeats until the player wins or loses.


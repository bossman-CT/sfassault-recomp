# Exit to Desktop (issue #57)

Game > Quality of life > Exit to Desktop from the main menu (`game.exit_to_desktop`,
`src/exit_to_desktop.cpp`), off by default. This is the issue's first option: an overlay
prompt, not an entry in the game's own menu, so the game's menu code and data are left alone.

With it on, while the title ("Press Start") or the main menu after it (Single Mode, VS Mode,
Options, Bonus Game) is up and taking input:

- A line at the bottom right names the button the player has: "Triangle: Exit to Desktop" on a
  PlayStation controller, "S: Exit to Desktop" with no controller on port 1, "Y: Exit to Desktop"
  otherwise (#74). It follows the port that last had a button pressed or a stick moved. SDL
  reports the controller's own type, and behind Steam Input (a virtual Xbox pad) SDL 2.30 reports
  the real one when Steam says what it is. The game's Y is the top face button in both Face
  buttons layouts, so the layout does not change the name.
- Y (the controller's top face button, Triangle on a PlayStation controller; S on the
  keyboard) asks "Quit the game?" in the menu's window, with the game paused and its pad at
  rest. Yes (A, Enter, a click) ends the run as the menu's Quit does, saving `settings.txt` and
  the input log. No, B, Esc, Start or Back goes back to the game.

Neither screen uses Y. Its press is taken out of the pad on those screens on the pad's way to
the game (an `input_ports` filter), which the input log records, so a replay needs nothing
more. The question itself is the frontend's and never reaches the game. With the option off
nothing is read or changed.

## gcn-recomp

The frontend part is generic, in the fork's `runtime/menu.h` (bossman-CT/gcn-recomp 042ad70):
`menu::hint` for a line over the picture, `menu::ask` for a yes/no question in the menu's
place, and `menu::quit` for the menu's own way out.

## Which screen is up

`game_mode::front_end()` (`src/game_mode.cpp`). The title and its menus are m2.rel (module
6). Its menu task, the one with update 0x11B9C into the module's code (0x81821C9C where
gcn-recomp places the module), keeps the screen it shows in the byte at +0x50, and +0x88 is 1
while that screen takes input. Logged through the `fortuna` route and a script that went to
player select and back:

| +0x50 | Screen |
|---|---|
| 0x02, 0x04 | the memory card check before the title |
| 0x5A | the title appearing |
| 0x5B | the title, waiting for Start (+0x88 = 1) |
| 0x5C | Start pressed, the main menu appearing |
| 0x5D | the main menu (+0x88 = 1; +0x53 is the cursor) |
| 0x5E, 0x5F | leaving it for the menus behind it |

B on player select goes back to the title (0x5A, then 0x5B), not to the main menu. Y on the
main menu did nothing in the original.

The task is found through the game's task tree (0x802B2900: the first task at +0, the next at
+0x18, the update at +0x20) and the SDK's module list (0x800030C8: id at +0, next at +4,
section table at +0x10, section 1's address at +8 of it).

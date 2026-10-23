# Controls

Either mouse button performs the same click on buttons, portraits, the 3D view,
combat rows and dialogs, including title and credits. In text entry, either
button has the same effect at the same position; use Escape to cancel.

| Key | Action |
| --- | --- |
| Up / Down, keypad 8 / 2 | Move forward/back outside combat |
| Left / Right, keypad 4 / 6 | Turn, also during combat |
| Ctrl+Left / Ctrl+Right | Strafe outside combat |
| W | Wait: advance time and monsters |
| Space | Interact outside combat, then Wait if the Event leaves Space in effect; acknowledge text |
| S / R / B | Shoot / Rest / Bash in exploration |
| D in exploration | Dismiss (not supported yet) |
| A / R / B in combat | Attack / Run / Block |
| C | Learned spells |
| U in combat | Use a charged item |
| F / O in combat | Quick Fight / Quick Fight Options |
| F1-F6 | Open a character sheet; choose a member in dialogs |
| I / Q / M / V | Info / Quick Reference / Map / Quests |
| = | Toggle the automap; Shift+= produces an unmatched + |
| Tab | Control panel: Save, Save As, Load, Quit |
| F9 | Save to the developer `--save-file` target |
| Escape | Back, cancel or acknowledge; in exploration, complete a pass |
| Arrows in the sheet | Select a stat |
| Enter | Open the selected stat; confirm a cast or text entry |
| Y / N | Yes / No in prompts |
| 1-9 | Select inventory or stock slots; 1-3 select combat rows; unmatched in exploration |
| I / E in the sheet | Items / Exchange with another member |
| W/A/C/M in Items | Weapons / Armor / Accessories / Misc |
| E / R / U in Items | Equip / Remove / Use; select a row then F1-F6 to transfer it |
| B / R in the Ironworks | Buy / Armor Repair |
| B / S / I / F in Smith Items | Buy / Sell / Identify / Fix; a row opens Y/N confirmation |
| T in Training | Train the selected member |
| H / D / U in the Temple | Heal / Donation / Uncurse |
| D / F / T / R in the Tavern | Drink / Food / Tip / Rumors |
| D / W in the Bank, then O / E | Deposit / Withdraw, then Gold / Gems |
| Digits, Backspace, Enter in Bank amount entry | Enter up to ten digits, edit, confirm; Escape cancels |
| Alt+Enter | Switch between window and fullscreen; no gameplay pass |

Exploration completes one Event pass after each input, including unmatched
keys such as period, U, digits, F7 and F8. Insert, Home, End and Page Up/Down
are also unmatched, as are Enter and Backspace in exploration. F8 speech replay
remains omitted.
Closing a dialog completes its opener's pass; responses inside it add none.
A 3D-view click runs the manual Event and then the pass, without Space's Wait
unless an Event acknowledgement replaces the current key.

Ctrl translates the plain DOS code before dispatch, overriding Shift:
Ctrl+I is Tab, Ctrl+grave is Space, Ctrl+H is Backspace, Ctrl+M is Enter and
Ctrl+[ is Escape. Ctrl+Up/Down are unmatched keys with a pass. Alt plus a key
acts as the plain key, except Alt+Enter. Keypad translation ignores NumLock;
keypad 0/1/3/7/9 are Insert/End/Page Down/Home/Page Up, and keypad 5 is 5.

F10 makes no pass; Ctrl+F10 is unmatched and does. F11, F12 and Delete
(including keypad decimal) produce no code anywhere. Shift, Ctrl, Alt and
Windows keys alone make no pass. Caps/Num/Scroll Lock produce unmatched
passes; releasing NumLock clears pending keys. Ordinary releases add no pass.
Held keys repeat after 500 ms at the existing nominal 10.9 Hz model; movement
retains the approved buffering that stops after the step in progress on release.

Button and portrait bounds include their left/top edges and exclude right/bottom.
The 3D view accepts x 8..224 and y 8..135. Clicking the panel icon's top rows
(y 137..140) runs the manual Event before opening the panel. Empty panel clicks,
mouse motion, window resize and window close add no pass.

Combat portraits show the remaining members after Run; either button or the
corresponding F key opens that member. The acting member and selected enemy
use the original icons.

# Update Script for Linux

This is a script that automatically opens gnome-terminal, konsole, xterm, lxterminal or terminator when executed, then displays the name of the package manager and performs an update of the system.
APT, DNF, zypper and Pacman are supported. After the update has been successfully completed, the terminal window is closed.

Before the script can be used, it must be made executable:
   ```bash
   chmod +x update.sh
   ```
The script is for people who are too stupid to update their system themselves and will never be able to, for example a friend who claims windows is better than linux, who you are trying to convince to switch to linux but have never been able to. (But there are probably even more ways to use the script.)

For easier execution, I recommend a desktop shortcut, for which I provide icons in /Icons .

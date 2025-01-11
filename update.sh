#!/bin/bash

# This part identifies the package manager
get_package_manager() {
  if command -v apt &> /dev/null; then
    echo "apt"
  elif command -v dnf &> /dev/null; then
    echo "dnf"
  elif command -v zypper &> /dev/null; then
    if grep -q "ID=opensuse-leap" /etc/os-release; then
      echo "zypper-leap"
    else grep -q "ID=opensuse-tumbleweed" /etc/os-release;
      echo "zypper-tumbleweed"
    fi
  elif command -v pacman &> /dev/null; then
    echo "pacman"
  else
    echo "unknown"
  fi
}

PACKAGEMANAGER=$(get_package_manager)

# Executes the update based on the recognized package manager
case "$PACKAGEMANAGER" in
  apt)
    echo "The system is being updated with APT..."
    COMMAND_TO_EXECUTE="sudo apt update && sudo apt upgrade -y"
    ;;
  dnf)
    echo "The system is being updated with DNF..."
    COMMAND_TO_EXECUTE="sudo dnf update --refresh -y"
    ;;
  zypper-leap)
    echo "The system is being updated with zypper (Leap)..."
    COMMAND_TO_EXECUTE="sudo zypper refresh && sudo zypper up -y"
    ;;
  zypper-tumbleweed)
    echo "The system is being updated with zypper (Tumbleweed)..."
    COMMAND_TO_EXECUTE="sudo zypper refresh && sudo zypper dup --allow-vendor-change -y"
    ;;
  pacman)
    echo "The system is being updated with Pacman..."
    COMMAND_TO_EXECUTE="sudo pacman -Syu --noconfirm"
    ;;
  unknown)
    echo "Unknown package manager. No update could be installed."
    ;;
  *)
    echo "Error: Unknown package manager '$PACKAGEMANAGER'."
    ;;
esac

# Identify and open the terminal and execute the update command
if command -v gnome-terminal >/dev/null 2>&1; then
  gnome-terminal -- bash -c "echo 'Using package manager: $PACKAGEMANAGER'; $COMMAND_TO_EXECUTE; echo 'Update completed. The window can be closed now.'; sleep 5; clear; echo 'Update finished. Closing terminal in 10 seconds...'; sleep 10; exit"
elif command -v konsole >/dev/null 2>&1; then
  konsole -e "bash -c \"echo 'Using package manager: $PACKAGEMANAGER'; $COMMAND_TO_EXECUTE; echo 'Update completed. The window can be closed now.'; sleep 5; clear; echo 'Update finished. Closing terminal in 10 seconds...'; sleep 10; exit\""
elif command -v xterm >/dev/null 2>&1; then
  xterm -e "bash -c 'echo \"Using package manager: $PACKAGEMANAGER\"; $COMMAND_TO_EXECUTE; echo \"Update completed. The window can be closed now.\"; sleep 5; clear; echo \"Update finished. Closing terminal in 10 seconds...\"; sleep 10; exit'"
elif command -v lxterminal >/dev/null 2>&1; then
  lxterminal -e "bash -c 'echo \"Using package manager: $PACKAGEMANAGER\"; $COMMAND_TO_EXECUTE; echo \"Update completed. The window can be closed now.\"; sleep 5; clear; echo \"Update finished. Closing terminal in 10 seconds...\"; sleep 10; exit'"
elif command -v terminator >/dev/null 2>&1; then
  terminator -e "bash -c 'echo \"Using package manager: $PACKAGEMANAGER\"; $COMMAND_TO_EXECUTE; echo \"Update completed. The window can be closed now.\"; sleep 5; clear; echo \"Update finished. Closing terminal in 10 seconds...\"; sleep 10; exit'"
else
  echo "No known terminal program found. Please install a terminal such as gnome-terminal, konsole, xterm, lxterminal, or terminator."
  exit 1
fi

exit 0

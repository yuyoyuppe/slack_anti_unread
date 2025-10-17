# Slack Anti-Unread

A lightweight background utility that prevents Slack's unread notification icon from being distracting when minimized to the system tray.

## What It Does

Slack displays a blue dot on its taskbar icon to indicate unread messages. This utility patches Slack to use the same icon whether you have unread messages or not, eliminating the visual distraction. Slack will still display a red dot icon in case you've got DMs or mentions.


## How It Works

- Replaces Slack's unread taskbar icon with the normal (rest) icon
- Automatically patches all installed Slack versions in `%LOCALAPPDATA%\slack`
- Monitors for new Slack installations and patches them automatically
- Preserves Slack's window state when restarting after patching
- Runs silently in the background

## Usage

Simply run it.
It's recommended to create a task in Task Scheduler with "At log on" trigger.

Logs are written to `%TEMP%\slack_anti_unread.log` with automatic rotation (max 1MB per file, 3 files total).

## Building

Requires:
- msvc 2022+
- vcpkg with WIL and spdlog installed
- premake5

```sh
premake5 vs2022
# Open build/slack_anti_unread.sln and build

```

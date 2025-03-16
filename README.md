# Discord CLI (dcli)

dcli is a command-line tool for sending messages and managing channels using a Discord Bot.

## Features

- Send Messages: Send messages to the selected channel.
- Channel Management: Add, remove, or switch channels.
- Retrieve User Messages: Fetch recent messages from a specific user.
- Configuration File: Save token and channel information in a configuration file.

## Requirements

- C++17 or later
- `libcurl` library
- `libssl` and `libcrypto` libraries
- `nlohmann/json` library

## Installation

1. Install Dependencies:
   - libcurl
   - nlohmann/json (C++ JSON library)

   Run the following command:
   sudo apt-get install libcurl4-openssl-dev

2. Build:
   Compile the program using:
   make

3. Install:
   Install the binary to your system:
   sudo make install

4. Uninstall:
   Remove the binary from your system:
   sudo make uninstall

## Usage

### Initial Setup

On the first run, you will need to provide your Discord Bot token and channel ID.

Example:
./dcli
Token: YOUR_DISCORD_BOT_TOKEN
Channel ID: YOUR_CHANNEL_ID
Enter a name for this channel: general

### Commands

- Send a Message:
  Example:
  > Hello, world!
  Message sent successfully!

- Switch Channel:
  Example:
  > /switch
  Select a channel to use:
  1. general
  2. random
  Enter the number of the channel: 2
  Switched to channel: random

- Check Current Channel:
  Example:
  > /channel
  Current channel: random

- Retrieve User Messages:
  Example:
  > /get
  Select a user to retrieve messages:
  1. user1
  2. user2
  Enter the number of the user: 1
  (Recent messages from the user will be displayed)

- Add a Channel:
  Example:
  > /add
  Enter Channel ID: NEW_CHANNEL_ID
  Enter Channel Name: new_channel
  Channel added successfully.

- Remove a Channel:
  Example:
  > /remove
  Select a channel to remove:
  1. general
  2. random
  Enter the number of the channel: 2
  Channel removed successfully.

- Exit:
  Example:
  > /exit

## Configuration File

The configuration file is saved at ~/.config/dcli/config.json. It has the following format:

{
    "token": "YOUR_DISCORD_BOT_TOKEN",
    "channels": {
        "general": "CHANNEL_ID_1",
        "random": "CHANNEL_ID_2"
    },
    "last_used_channel": "CHANNEL_ID_1"
}

## License

This project is licensed under the MIT License. See the LICENSE file for details.
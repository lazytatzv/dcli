# dcli

## Description
`dcli` is a simple command-line interface (CLI) application that allows you to send messages to a Discord channel directly from your terminal.

## Features
- Send messages to Discord using your bot's token.
- Specify the target channel using the channel ID.
- Configure token and channel ID through a configuration file or command-line arguments.
- Supports message input directly from the terminal.

## Installation

To install `dcli`, you can follow these steps:

1. Clone the repository:
   ```bash
   git clone https://github.com/yourusername/dcli.git
   cd dcli
   ```

2. Build the application using `make`:
   ```bash
   make
   ```

3. Install it to your system:
   ```bash
   sudo make install
   ```

Once installed, you can run `dcli` from your terminal to send messages to a Discord channel.

## Usage

### Command-line options:

- `--token <TOKEN>`: Set your Discord bot's token.
- `--channel-id <CHANNEL_ID>`: Set the target Discord channel ID.
- `--help`: Show the help message.

### Example:

To send a message to a channel, run:
```bash
dcli --token YOUR_BOT_TOKEN --channel-id CHANNEL_ID
```
You will be prompted to enter your message, and it will be sent to the specified channel.

## License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

#!/bin/bash

set -e # in case of error, exit pipeline

# Update apt
apt-get update
cd ~

# Install arduino-cli
apt-get install curl -y
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh # Download and execute Arduino CLI install script from official GitHub
export PATH=$PATH:/root/bin
arduino-cli version

# Add Seeed Studio boards to Arduino CLI (creates a config file)
echo "board_manager:
  additional_urls:
    - https://files.seeedstudio.com/arduino/package_seeeduino_boards_index.json" >.arduino-cli.yaml

# update index to work w/ board manager and Seeduino SAMD microchip dependencies, add into config
arduino-cli core update-index --config-file .arduino-cli.yaml
arduino-cli core install Seeeduino:samd --config-file .arduino-cli.yaml

# should be fine without versioning, installs latest version, if this causes major issues, apply versions manually
arduino-cli lib install "lvgl"
arduino-cli lib install "Grove Temperature and Humidity Sensor"
arduino-cli lib install "pubsubclient"
arduino-cli lib install "Seeed Arduino FS"
arduino-cli lib install "Seeed Arduino SFUD"
arduino-cli lib install "Seeed Arduino rpcUnified"
arduino-cli lib install "Seeed Arduino rpcWiFi"
arduino-cli lib install "Seeed_Arduino_mbedtls"

cd - # Return to previous working directory

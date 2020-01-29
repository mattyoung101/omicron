#!/usr/bin/fish
echo "!!! Warning: Please make sure you have the correct disk set in this file. Press CTRL+C to abort now if you need to."
read -p "$argv"
sudo dd bs=4M if=/dev/sdd status=progress | gzip > ~/jetson_backup.img.gz
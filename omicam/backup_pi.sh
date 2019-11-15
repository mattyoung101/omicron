#!/usr/bin/fish
sudo dd bs=4M if=/dev/sdd status=progress | gzip > ~/raspibackup.img.gz
#!/usr/bin/env bash

MAP_DIR=$(mktemp -d -t map-XXXXXXXXXX)
TARGET_USER=ubuntu2004-eugene
TARGET_GROUP=ubuntu2004-eugene
CONTAINER=ubuntu20.04
PROJECT=$(realpath $(dirname $0)/..)
CONTAINER_USER=eugene
CONTAINER_PROJECT="/home/$CONTAINER_USER/$(basename $PROJECT)"
SSH_KEY=$HOME/.ssh/id_rsa

# Map project and start container
echo "Setting up container..."
mkdir -p $MAP_DIR
sudo -i -- sh -c "echo 'Remapping directory ownership...' && bindfs --map=$(id -nu)/$TARGET_USER:@$(id -ng)/@$TARGET_GROUP $PROJECT $MAP_DIR && \
	echo 'Starting container...' && systemd-nspawn -nbUD /var/lib/machines/$CONTAINER --bind=$MAP_DIR:$CONTAINER_PROJECT --console=passive >/dev/null 2>&1 &"
sleep 1

# Forward ports
echo "Forwarding ports..."
CONTAINER_IP=$(sudo systemd-run --machine=$CONTAINER -P /sbin/ip -4 addr show host0 | grep -oP '(?<=inet\s)\d+(\.\d+){3}')
if [[ "x$(ssh-add -l | grep $SSH_KEY)" == "x" ]]; then
	ssh-add $SSH_KEY
fi
ssh -NL 4567:localhost:4567 $CONTAINER_IP &
ssh -NR 1234:localhost:1234 $CONTAINER_IP &

# Listen for CTRL+C
trap shutdown INT
trap shutdown HUP
function shutdown() {
	echo "Stopping port forwarding..."
	kill $(jobs -p)
	echo "Stopping container..."
	sudo -i -- sh -c "machinectl stop $CONTAINER >/dev/null 2>&1 && umount $MAP_DIR"
	rm -rf $MAP_DIR
	echo "Shutdown"
}

echo "Ready..."
sudo machinectl shell --setenv="TERM=xterm-256color" $CONTAINER_USER@$CONTAINER
shutdown
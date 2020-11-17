## Building a Raspberry Pi OS image

This process creates an SD card image, bootstrapping it with our custom configuration.
It's intended to eventually be the method that we create SD cards that would
ship with the pitrex.

The process uses Hashicorp Vagrant to automate a linux VM that then calls
Hashicorp Packer with the Raspberry Pi OS builder from 
https://github.com/solo-io/packer-builder-arm-image

For now the image isn't really suitable for end users - we need some kind of
menu system, and a likely a way of setting wifi details via this menu. Right
now though, it does allow pitrex devs to build their own custom image, and I'm
hoping encourage more work in this area.

If you have any questions, please reach out to Mike Pountney on the pitrex-dev
mailing list.

## Procedure

### Install Vagrant

See https://www.vagrantup.com/downloads

### Set environment for Wifi/SSH setup:

   export WIFI_NAME={your wifi ssid}
   export WIFI_PASS={your wifi password}

   # optional, so you can log in as pi without a password
   export SSH_PUBKEY={your ssh public key}


### Initial provisioning

   vagrant up

### Recreating the image

   vagrant provision --provision-with build-image


### Burn image!

This will create an image that can be burned to a SD card:

   output-arm-image.img

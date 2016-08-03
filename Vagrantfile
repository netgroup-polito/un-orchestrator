# -*- mode: ruby -*-

$script = <<SCRIPT
dnf -y copr enable bisdn/rofl
dnf -y install \
  autoconf \
  automake \
  binutils \
  boost-devel \
  cmake \
  ethtool \
  gcc-c++ \
  glibc-devel \
  libmicrohttpd-devel \
  libtool \
  libxml2-devel \
  make \
  rofl-common \
  rofl-common-devel \
  openssl-devel \
  sqlite-devel \
  patch \
  unzip
SCRIPT

Vagrant.configure(2) do |config|
  config.vm.box = "base"
  config.vm.box = "fedora/23-cloud-base"
  config.vm.synced_folder "./", "/vagrant", type: "nfs"
  config.vm.provision "shell", inline: $script

  config.vm.provider :libvirt do |libvirt, override|
    libvirt.memory = 2048
    libvirt.cpus = 2
  end
end

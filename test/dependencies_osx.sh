#!/bin/bash
#  ----------------------------------------------------------------
# Copyright 2016 Cisco Systems
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ------------------------------------------------------------------
# This file has been modified by Yan Gorelik, YDK Solutions.
# All modifications in original under CiscoDevNet domain
# introduced since October 2019 are copyrighted.
# All rights reserved under Apache License, Version 2.0.
# ------------------------------------------------------------------
#
# dependencies_osx.sh
# Script for running ydk CI on docker via travis-ci.org
# and installing YDK from install_ydk.sh script
# ------------------------------------------------------------------

function print_msg {
    echo -e "${MSG_COLOR}*** $(date) *** dependencies_osx.sh | $@ ${NOCOLOR}"
}

function run_cmd {
    local cmd=$@
    print_msg "Running: $cmd"
    $@
    local status=$?
    if [ $status -ne 0 ]; then
        MSG_COLOR=$RED
        print_msg "Exiting '$@' with status=$status"
        exit $status
    fi
    return $status
}

function install_libssh {
    print_msg "Checking installation of libssh"
    locate libssh_threads.dylib
    local status=$?
    if [[ ${status} == 0 ]]; then
        return
    fi
    print_msg "Installing libssh-0.7.6"
    brew reinstall openssl
    export OPENSSL_ROOT_DIR=/usr/local/opt/openssl
    wget https://git.libssh.org/projects/libssh.git/snapshot/libssh-0.7.6.tar.gz
    tar zxf libssh-0.7.6.tar.gz && rm -f libssh-0.7.6.tar.gz
    mkdir libssh-0.7.6/build && cd libssh-0.7.6/build
    cmake ..
    sudo make install
    cd -
}

function install_confd {
  if [[ ! -s $HOME/confd/bin/confd ]]; then
    if [[ $os_version < "10.14" ]]
      print_msg "Installing confd-basic-6.2"
      wget https://github.com/CiscoDevNet/ydk-gen/files/562559/confd-basic-6.2.darwin.x86_64.zip &> /dev/null
      unzip confd-basic-6.2.darwin.x86_64.zip
      ./confd-basic-6.2.darwin.x86_64.installer.bin $HOME/confd
    else
      print_msg "Installing confd-basic-7.3"
      unzip $curr_dir/3d_party/darwin/confd-basic-7.3.darwin.x86_64.zip
      cd confd-basic-7.3.darwin.x86_64
      run_cmd ./confd-basic-7.3.darwin.x86_64.installer.bin $HOME/confd
      cd -
    fi
    rm -rf confd-basic-* ConfD*
  fi
}

function install_golang {
  go_exec=$(which go)
  if [[ -z ${go_exec} && -d /usr/local/go ]]; then
    go_exec=/usr/local/go/bin/go
  fi
  if [[ -x ${go_exec} ]]
  then
    go_version=$(echo `${go_exec} version` | awk '{ print $3 }' | cut -d 'o' -f 2)
    print_msg "Current Go version is $go_version"
    minor=$(echo $go_version | cut -d '.' -f 2)
  else
    print_msg "The Go is not installed"
    minor=0
  fi
  if (( $minor < 9 )); then
    print_msg "Installing Go1.9.2"
    bash < <(curl -s -S -L https://raw.githubusercontent.com/moovweb/gvm/master/binscripts/gvm-installer)
    source /Users/travis/.gvm/scripts/gvm
    print_msg "GO version before installation: $(go version)"
    gvm install go1.4 -B
    gvm use go1.4
    export GOROOT_BOOTSTRAP=$GOROOT
    gvm install go1.9.2
    gvm use go1.9.2
    print_msg "GOROOT: $GOROOT"
    print_msg "GOPATH: $GOPATH"
    print_msg "GO version: $(go version)"
    print_msg " "
  fi
}

function check_python_installation {
  print_msg "Checking python and pip installation"
  python3 -V
  status=$?
  if [ $status -ne 0 ]; then
    print_msg "Installing python3"
    brew install python
  fi
  pip3 -V
  status=$?
  if [ $status -ne 0 ]; then
    print_msg "Installing pip${PYTHON_VERSION}"
    run_cmd curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py
    run_cmd sudo -H python3 get-pip.py
  fi
}

########################## EXECUTION STARTS HERE #############################

# Terminal colors
NOCOLOR='\033[0m'
YELLOW='\033[1;33m'
MSG_COLOR=$YELLOW

os_version=$(sw_vers | grep ProductVersion | awk '{print$2}')
curr_dir=$(pwd)

brew install curl doxygen xml2
brew install pybind11 valgrind

install_libssh
install_confd
install_golang

check_python_installation

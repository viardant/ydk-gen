..
  #  YDK-YANG Development Kit
  #  Copyright 2016-2019 Cisco Systems. All rights reserved
  # *************************************************************
  # Licensed to the Apache Software Foundation (ASF) under one
  # or more contributor license agreements.  See the NOTICE file
  # distributed with this work for additional information
  # regarding copyright ownership.  The ASF licenses this file
  # to you under the Apache License, Version 2.0 (the
  # "License"); you may not use this file except in compliance
  # with the License.  You may obtain a copy of the License at
  #
  #   http:#www.apache.org/licenses/LICENSE-2.0
  #
  #  Unless required by applicable law or agreed to in writing,
  # software distributed under the License is distributed on an
  # "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
  # KIND, either express or implied.  See the License for the
  # specific language governing permissions and limitations
  # under the License.
  # *************************************************************
  # This file has been modified by Yan Gorelik, YDK Solutions.
  # All modifications in original under CiscoDevNet domain
  # introduced since October 2019 are copyrighted.
  # All rights reserved under Apache License, Version 2.0.
  # *************************************************************

===============
Getting Started
===============
.. contents:: Table of Contents

Overview
========

The YANG Development Kit (YDK) is a software development tool, which provides API for building applications based on YANG models.
The main goal of YDK is to reduce the learning curve of YANG data models by expressing the model semantics in an API
and abstracting protocol/encoding details.  YDK is composed of a core package that defines services and providers,
plus one or more module bundles that are based on YANG models.

Backward Compatibility
======================

The Python YDK-0.8.5 core package is compatible with all model bundles generated previously with ydk-gen releases starting from 0.7.3.
However the YDK-0.8.5 generates slightly different code and model API comparing to YDK-0.8.4.
The YDK-0.8.5 generated code is not compatible with YDK-0.7.2 and earlier bundle packages due to changes in modeling and handling YList objects.

Docker
======

A `docker image <https://docs.docker.com/engine/reference/run/>`_ is automatically built with the latest ydk-gen installed.
This be used to run ydk-gen without installing anything natively on your platform.

To use the docker image, `install docker <https://docs.docker.com/install/>`_ on your system and run the below command.
See the `docker documentation <https://docs.docker.com/engine/reference/run/>`_ for more details::

  docker run -it ydksolutions/ydk-gen


System Requirements
===================

The YDK is currently supported on the following platforms including native installations, virtual machines, and docker images:

 - Linux Ubuntu Xenial (16.04 LTS), Bionic (18.04 LTS), and Focal (20.04 LTS)
 - Linux CentOS/RHEL versions 7 and 8
 - MacOS up to 10.14.6 (Mojave)

On Windows 10 the Linux virtual machine can run using Windows Subsystem for Linux (WSL);
check `this <https://www.windowscentral.com/install-windows-subsystem-linux-windows-10>`_ for virtual machine installation procedure.
The YDK has been tested in such environment on Ubuntu Bionic (18.04 LTS) and Focal (20.04 LTS) images obtained
from Microsoft Store.

On supported platforms the YDK can be installed using `Installation Script`_.
On other platforms the YDK should be installed manually `Building from source`_.
For both the methods the user must install `git` package prior to the installation procedure.

All YDK core components are based on C and C++ code. These components compiled using default compilers for the supported platform.
Corresponding binaries, libraries, and header files are installed in default locations,
which are `/usr/local/bin`, `/usr/local/lib`, and `/usr/local/include`.
The user must have sudo access in order to install YDK core components to these locations.

.. _howto-install:

Core Installation
=================

Installation Script
-------------------

For YDK installation it is recommended to use script `install_ydk.sh` from `ydk-gen` git repository.
The script detects platform OS, installs all the dependencies and builds complete set of YDK components for specified language.
The user must have sudo access to these locations.

The YDK extensively uses Python scripts for building its components and model API packages (bundles).
In order to isolate YDK Python environment from system installation, the script builds Python3 virtual environment.
The user must manually activate virtual environment when generating model bundles and/or running YDK based application.
By default the Python virtual environment is installed under `$HOME/venv` directory.
If user has different location, the PYTHON_VENV environment variable should be set to that location.

Here is simple example of core YDK installation for Python programming language:

.. code-block:: sh

    git clone https://github.com/ygorelik/ydk-gen.git
    cd ydk-gen
    export YDKGEN_HOME=`pwd`  # optional
    export PYTHON_VENV=$HOME/ydk_vne  # optional
    ./install_ydk.sh --core


The script also allows to install individual components like dependencies, core, and service packages
for specified programming language or for all supported languages.
Full set of script capabilities could be viewed like this::

    ./install_ydk.sh --help
    usage: install_ydk [-l [cpp, py, go]] [-s gnmi] [-h] [-n]
    Options and arguments:
      -l [cpp, py, go, all] installation language; if not specified Python is assumed
                            'all' corresponds to all available languages
      -c|--core             install YDK core package
      -s|--service gnmi     install gNMI service package
      -n|--no-deps          skip installation of dependencies
      -h|--help             print this help message and exit

    Environment variables:
    YDKGEN_HOME         specifies location of ydk-gen git repository;
                        if not set, $HOME/ydk-gen is assumed
    PYTHON_VENV         specifies location of python virtual environment;
                        if not set, /home/ygorelik/venv is assumed
    GOROOT              specifies installation directory of go software;
                        if not set, /usr/local/go is assumed
    GOPATH              specifies location of go source directory;
                        if not set, $HOME/go is assumed
    C_INCLUDE_PATH      location of C include files;
                        if not set, /usr/local/include is assumed
    CPLUS_INCLUDE_PATH  location of C++ include files;
                        if not set, /usr/local/include is assumed


If user environment is different from the default one (different Python installation or different
location of libraries), then building from source method should be used.

Building from source
--------------------

Installing third party dependencies
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If user platform is supported one, it is recommended to use `ydk-gen/install_ydk.sh` script.
The script will also install Python virtual environment in default or specified location::

    # Clone ydk-gen from GitHub
    git clone https://github.com/ygorelik/ydk-gen.git
    cd ydk-gen

    # Define optional environment variables and install dependencies
    export YDKGEN_HOME=`pwd`
    export PYTHON_VENV=$HOME/ydk_venv
    ./install_ydk.sh   # also builds Python virtual environment

For unsupported platforms it is recommended to follow logic of `ydk-gen/test/dependencies-*` scripts.

Environment variables
~~~~~~~~~~~~~~~~~~~~~

In some OS configurations during YDK package installation the cmake fails to find C/C++ headers for previously installed YDK libraries.
In this case the header location must be specified explicitly (in below commands the default location is shown)::

  export C_INCLUDE_PATH=/usr/local/include
  export CPLUS_INCLUDE_PATH=/usr/local/include

Installing core components
~~~~~~~~~~~~~~~~~~~~~~~~~~

::

    # Activate Python virtual environment
    source $PYTHON_VENV/bin/activate

    # Generate and install YDK core library
    ./generate.py -is --core --cpp

    # For Python programming language add
    ./generate.py -i --core

    # For Go programming language add
    ./generate.py -i --core --go


Adding gNMI Service
-------------------

In order to enable YDK support for gNMI protocol, which is optional, the user need install third party software
and YDK gNMI service package.

gNMI Service installation
~~~~~~~~~~~~~~~~~~~~~~~~~

Here is simple example, how gNMI service package for Python could be added::

    cd ydk-gen
    ./install_ydk.sh -l py --service gnmi


gNMI runtime environment
~~~~~~~~~~~~~~~~~~~~~~~~

There is an open issue with gRPC on Centos/RHEL, which requires an extra step before running any YDK gNMI application.
See this issue on `GRPC GitHub <https://github.com/grpc/grpc/issues/10942#issuecomment-312565041>`_ for details.
As a workaround, the YDK based application runtime environment must include setting of `LD_LIBRARY_PATH` variable::

    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:~/grpc/libs/opt:~/protobuf-3.5.0/src/.libs:/usr/local/lib:/usr/local/lib64


Bundle Installation
===================

Quick installation
------------------

You can install the latest model packages from Python package index.
Note that Python index does not have this release, therefore make sure the Python core package for this release is already installed prior to bundle installation.
Make sure to activate Python virtual environment prior to package installation.
When installing a bundle package from Python index, all dependent packages installed automatically.
The installation of the `ydk-models-cisco-ios-xr` and/or `ydk-models-cisco-ios-xe` bundle(s)
(depending on whether you're developing application for IOS XR or IOS XE platform) automatically installs all other
related packages (`ydk`, `openconfig` and `ietf` packages)::

  pip install ydk-models-cisco-ios-xr
  pip install ydk-models-cisco-ios-xe

Alternatively, you can perform a partial installation.
If you prefer to install only the `openconfig` bundle and its dependencies (`ydk` and `ietf` packages), execute::

  pip install ydk
  pip install ydk-models-openconfig

If you want install only the `ietf` bundle and its dependencies (`ydk` package), execute::

  pip install ydk
  pip install ydk-models-ietf

To enable gNMI Service support in Python based application, install package::

  pip install ydk-service-gnmi


Installing from source
----------------------

Once you have installed the `ydk` core package, you can install one or more model bundles.  Note that some bundles have dependencies on other bundles.
Those dependencies are already captured in the bundle package.  Make sure you install the desired bundles in the order below.
To install the `ietf` bundle from `ydk-gen` execute::

  # Activate Python virtual environment and navigate to ydk-gen directory
  source $PYTHON_VENV/bin/activate
  cd ydk-gen
  # Generate and install the bundle
  ./generate.py --bundle profiles/bundles/ietf_0_1_5_post2.json -i

To install the `openconfig` bundle, execute::

  # Activate Python virtual environment and navigate to ydk-gen directory
  source $PYTHON_VENV/bin/activate
  cd ydk-gen
  # Generate and install the bundle
  ./generate.py --bundle profiles/bundles/openconfig_0_1_8.json -i


To install the `cisco-ios-xr` bundle, execute::

  # Activate Python virtual environment and navigate to ydk-gen directory
  source $PYTHON_VENV/bin/activate
  cd ydk-gen
  # Generate and install the bundle
  ./generate.py --bundle profiles/bundles/cisco-ios-xr-6_6_3_post1.json -i


Generate YDK components
=======================

Generation script
-----------------

All the YDK components/packages can be generated by using Python script `generate.py`. To get all of its options run::

    cd ydk-gen
    ./generate.py --help
    usage: generate.py [-h] [-l] [--core] [--service SERVICE] [--bundle BUNDLE]
                       [--adhoc-bundle-name ADHOC_BUNDLE_NAME]
                       [--adhoc-bundle ADHOC_BUNDLE [ADHOC_BUNDLE ...]]
                       [--generate-meta] [--generate-doc] [--generate-tests]
                       [--output-directory OUTPUT_DIRECTORY] [--cached-output-dir]
                       [-p] [-c] [-g] [-v] [-o]

    Generate YDK artifacts:

    optional arguments:
      -h, --help            show this help message and exit
      -l, --libydk          Generate libydk core package
      --core                Generate and/or install core library
      --service SERVICE     Location of service profile JSON file
      --bundle BUNDLE       Location of bundle profile JSON file
      --adhoc-bundle-name ADHOC_BUNDLE_NAME
                            Name of the adhoc bundle
      --adhoc-bundle ADHOC_BUNDLE [ADHOC_BUNDLE ...]
                            Generate an SDK from a specified list of files
      --generate-meta       Generate meta-data for Python bundle
      --generate-doc        Generate documentation
      --generate-tests      Generate tests
      --output-directory OUTPUT_DIRECTORY
                            The output directory where the sdk will get created.
      --cached-output-dir   The output directory specified with --output-directory
                            includes a cache of previously generated gen-
                            api/<language> files under a directory called 'cache'.
                            To be used to generate docs for --core
      -p, --python          Generate Python SDK
      -c, --cpp             Generate C++ SDK
      -g, --go              Generate Go SDK
      -v, --verbose         Verbose mode
      -o, --one-class-per-module
                            Generate separate modules for each python class
                            corresponding to YANG containers or lists.

Build model bundle profile
--------------------------

The first step in using ydk-gen is either using one of the already built `bundle profiles <https://github.com/ygorelik/ydk-gen/tree/master/profiles/bundles>`_
or constructing your own bundle profile, consisting of the YANG models you are interested to include into the bundle.

Construct a bundle profile file, such as `cisco-ios-xr_6_5_3 <https://github.com/ygorelik/ydk-gen/blob/master/profiles/bundles/cisco-ios-xr_6_5_3.json>`_
and specify its dependencies.

A sample bundle profile file is described below. The file is in a JSON format. The profile must define the "name",
"version" and "description" of the bundle, and then the "core_version", which refers to
`the version <https://github.com/ygorelik/ydk-gen/releases>`_ of the YDK core package that you want to use with this bundle.
The "name" of the bundle will form part of the installation path of the bundle.
All other attributes, like "author" and "copyright", are optional and will not affect the bundle generation::

    "name":"cisco-ios-xr",
    "version": "6.5.3",
    "core_version": "0.8.5",
    "author": "Cisco",
    "copyright": "Cisco",
    "description": "Cisco IOS-XR Native Models From Git",

The `models` section of the profile describes sources of YANG models. It could contain combination of elements:

- `dir` - list of **relative** directory paths containing YANG files
- `file` - list of **relative** YANG file paths
- `git` - git repository, where YANG files are located

The sample below shows the use of git sources only.
Each `git` source must specify `url` - git repository URL, and `commits` list.
The specified URL must allow the repository to be cloned without user intervention.
Each element in `commits` list can specify:

- `commitid` - optional specification of a commit ID in string format. If not specified the HEAD revision is assumed.
- `dir` - optional list of **relative** directory paths within the git repository.
- `file` - optional list of **relative** `*.yang` file paths within the git repository.

Only directory examples are shown in this example::


    "models": {
        "git": [
            {
                "url": "https://github.com/YangModels/yang.git",
                "commits": [
                  {
                    "dir": [
                        "vendor/cisco/xr/653"
                    ]
                  }
                ]
            },
            {
                "url": "https://github.com/YangModels/yang.git",
                "commits": [
                  {
                    "commitid": "f6b4e2d59d4eedf31ae8b2fa3119468e4c38259c",
                    "dir": [
                        "experimental/openconfig/bgp",
                        "experimental/openconfig/policy"
                    ]
                  }
                ]
            }
        ]
    },

Generate and install model bundle
---------------------------------

Generate model bundle using a bundle profile and install it.
Python virtual environment must be activated prior to these procedures::

    ./generate.py --python --bundle profiles/bundles/<name-of-profile>.json -i

Check Python packages installed::

    pip list | grep ydk
    ydk (0.8.5)
    ydk-models-<name-of-bundle> (0.5.1)
    ...

Generate "adhoc" bundle
-----------------------

When YANG models available on the hard drive, there is capability to generate small model bundles, which include
just few models. It is called an "adhoc" bundle. Such a bundle generated without profile directly from command line.
Here is simple example::

    ./generate.py --adhoc-bundle-name test --adhoc-bundle \
        /opt/git-repos/clean-yang/vendor/cisco/xr/621/Cisco-IOS-XR-ipv4-bgp-oper*.yang \
        /opt/git-repos/clean-yang/vendor/cisco/xr/621/Cisco-IOS-XR-types.yang
        /opt/git-repos/clean-yang/vendor/cisco/xr/621/Cisco-IOS-XR-ipv4-bgp-datatypes.yang

This will generate a bundle that contains files specified in the `--adhoc-bundle` option and
create Python package `ydk-models-test-0.1.0.tar.gz`, which has dependency on the base IETF bundle.
Note that **all** dependencies for the bundle must be listed. It is expected that this option will be typically used
for generating point model bundles for specific testing. The `--verbose` option is automatically enabled to quickly
and easily let the user see if dependencies have been satisfied.

Generate bundle documentation
-----------------------------

In order to generate YDK core and bundles documentation, the `--generate-doc` option is used when generating core package.
Therefore the user should generate all the bundles without the `--generate-doc` option prior to the documentation generation.
For example, the below sequence of commands will generate the documentation for the three python bundles and the python core::

    ./generate.py --python --bundle profiles/bundles/ietf_0_1_1.json
    ./generate.py --python --bundle profiles/bundles/openconfig_0_1_1.json
    ./generate.py --python --bundle profiles/bundles/cisco_ios_xr_6_1_1.json
    ./generate.py --python --core --generate-doc

**Note.** The documentation generation for bundles can take few hours due to their sizes.
If you have previously generated documentation using the `--cached-output-dir --output-directory <dir>` option,
the add-on documentation generation time can be reduced. Adding cisco-ios-xr documentation as an example::

    mkdir gen-api/cache
    mv gen-api/python gen-api/cache

    ./generate.py --python --bundle profiles/bundles/cisco_ios_xr_6_6_3.json
    ./generate.py --python --core --generate-doc --output-directory gen-api --cached-output-dir

Pre-generated documentation for YDK-0.8.3 core and model API for most popular devices is available
`online <http://ydk.cisco.com>`_.

Documentation and Support
=========================

- Application samples can be found under the `samples directory <https://github.com/CiscoDevNet/ydk-py/tree/master/core/samples>`_
- Hundreds of Python application samples can be found in the `samples repository <https://github.com/CiscoDevNet/ydk-py-samples>`_
- Join the `YDK community <https://communities.cisco.com/community/developer/ydk>`_ to connect with YDK users and developers

Release Notes
=============

The current YDK release version is 0.8.5.3.
YDK-Gen is licensed under the Apache 2.0 License.

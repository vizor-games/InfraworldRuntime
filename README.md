Vizor Infraworld
================

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
[![Maintainability](https://api.codeclimate.com/v1/badges/d8740022fdc1bbc8277b/maintainability)](https://codeclimate.com/github/vizor-games/InfraworldRuntime/maintainability)

Welcome to the Infraworld source code!

![](icon.png)

Infraworld is a solution that enables [Unreal Engine 4](https://www.unrealengine.com/en-US) to work with [Google gRPC](https://gRPC.io) services using either **C++** or **Blueprints**.

Infraworld is a fast, robust and cross platform.
It fits any stage of development: either prototyping or production. Saving a tons of your team's time, you need to write your gRPC wrappers by hand no more.
[A special converter utility](https://github.com/vizor-games/infraworld-cornerstone) will do it for you, producing high quality, debuggable and multi-threaded code, gaining lowest possible overhead to your game logic thread.
You may also work with either generated or shipped with gRPC C functions and C++ classes in your own way, even completely ignoring runtime classes, since the InfraworldRuntime adds all required headers and wires all required libraries.

Also, you may want to use a [protobuild](https://github.com/vizor-games/infraworld-protobuild) utility to automate cross-language gRPC wrapper generation.


Getting started
===============

##### Building gRPC support

At the first step, you need to build gRPC runtime libraries.
Just run `Setup.sh` for Linux, `Setup.bat` for Windows or `Setup.command` for macOS (please don't use `Setup.sh` on macOS, because Linux and macOS build pipelines are completely different!). OR you may want to use our sweet [pre-compiled binaries](../../releases) to avoid manual building and save our planet from carbon emission disaster! The runtime uses gRPC branch `v1.15.x`.

* For Windows, we recommend you to use [chocolatey](https://chocolatey.org) to install packages into your system.
**Note** that you do need all these programs in your system's `PATH` ([See how to edit PATH on Windows](https://www.computerhope.com/issues/ch000549.htm)):
  * [Git VCS](https://git-scm.com/download/win)
  * [Visual Studio 2017](https://visualstudio.microsoft.com/downloads/) with VC++ tools v141 installed
  * [CMake](https://cmake.org) is used to generate a Visual Studio solution from the `CMakeLists.txt` provided with gRPC
  * [Strawberry perl](http://strawberryperl.com) 64bit version
  * [NASM](https://www.nasm.us)
  * [Golang](https://golang.org/doc/install)
* For any distribution of Linux and for macOS systems you need (use `apt`, `pacman`, `emerge` or any other package manager to install this software):
  * git
  * automake, autoconf and libtool
  * make
  * strip
  * go
  * [Unreal Engine 4.20 installed](https://github.com/EpicGames/UnrealEngine/tree/4.20), additionally you need to `export UE_ROOT=/path/to/root/ue4/directory`, because you need UE4 to build GRPC for linux.
* For macOS (use `homebrew` or `macports` to install this software):
  * git
  * xCode 10.0+
  * go

**Note** that required programs for Linux and MacOS systems are being checked in run-time.

Then you may (or may not) import `GrpcIncludes` and `GrpcIncludes` folders into your VCS, but you need to build them manually at least one time for each platform.
The build process requires an access to the Internet.

##### Installing the plugin
Just copy the resulting folder into the your project’s Plugins folder (create it if you don’t have one).
Then, after that project is being opened, a dialog box, telling that the plugin is need to be compiled should appear. Then confirm the dialog by clicking `Yes`.

##### Building and the converter
Please take a look at the [infraworld-cornerstone documentation](https://github.com/vizor-games/infraworld-cornerstone) for details.

##### Using generated code.
Please take a look at the [example project](https://drive.google.com/open?id=13EZzP_9033vBC7VzJf9LFrygg42LHaOW) for tutorial.

Running the example project
===========================

You should copy built plugin's folder into `InfraworldExample/Plugins` folder.
Then just open `InfraworldExample.uproject`. Server code is in `InfraworldExample/Server` folder.
You are required to install dependencies using `pip` and `requirements.txt` file.

Debugging
=========

Since the plugin itself is an open source software, you may want to debug it or add some extra functionality.
Since it is distributed as an Unreal Engine plugin, you can add it into your own game
and then generate **Visual Studio solution**, **XCode project** or **CMakeLists**. Use `Development` or `DebugGame` run configuration!

Contribution
============

Please feel free to report known bugs, propose new features and improve tests using Github's pull request system.
Please do not add either build libraries for an any platform or header files into your commits. Thank you very much for contributing into free software.

References
==========
* [Introduction to UE4 Plugins](https://wiki.unrealengine.com/An_Introduction_to_UE4_Plugins)
* [gRPC API docs](https://gRPC.io/docs/)

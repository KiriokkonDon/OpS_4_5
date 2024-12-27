#!/bin/bash

PTY_OPTIONS="pty,raw,echo=0"

socat -d -d $PTY_OPTIONS $PTY_OPTIONS

#!/usr/bin/env bash

netcat -ulvvp 15708 n3ds | hexdump -f packet.hxfmt

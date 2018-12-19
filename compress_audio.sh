#!/bin/bash

# Sampling frequency: 8820Hz
# Number of channels: 1
# Sampling resolution: 8-bit
ffmpeg -i $1 -ar 8820 -ac 1 -c:a pcm_u8 $2

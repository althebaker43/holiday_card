#!/bin/bash

avrdude -c usbtiny -p m328p -U flash:w:holiday_card.hex
